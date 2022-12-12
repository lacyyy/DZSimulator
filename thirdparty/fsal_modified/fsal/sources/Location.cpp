#include "fsal.h"

#include <stdio.h>  /* defines FILENAME_MAX */
#ifdef _WIN32
#include <direct.h>
#include <Shlobj.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#undef GetCurrentDirectory

using namespace fsal;

path Location::GetCurrentDirectory()
{
	static char cCurrentPath[FILENAME_MAX];
	GetCurrentDir(cCurrentPath, sizeof(cCurrentPath));
	cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';
	return cCurrentPath;
}

Location::Location(const char* filepath, Options location, PathType type, LinkType link) : m_filepath(fs::u8path(filepath)), m_relartiveTo(location), m_type(type), m_link(link)
{
}

Location::Location(const std::string& filepath, Options location, PathType type, LinkType link) : m_filepath(fs::u8path(filepath)), m_relartiveTo(location), m_type(type), m_link(link)
{
}

Location::Location(const path& filepath, Options location, PathType type, LinkType link) : m_filepath(filepath), m_relartiveTo(location), m_type(type), m_link(link)
{
}

Location::Location(Options location) : m_relartiveTo(location), m_type(kFile | kDirectory), m_link(kSymlink | kNotSymlink)
{
}

path Location::GetSytemPath(Options relartiveTo)
{
	switch (relartiveTo)
	{
#ifdef WIN32
	case kTemp:
		char buff[1024];
		GetTempPath(1024, buff);
		return path(buff);
#endif
#ifdef __linux__
	case kTemp:
	{
		#if __cplusplus >= 201402L
		return fs::temp_directory_path();
		#else
		char const* folder = getenv("TMPDIR");
		fs::temp_directory_path();
		if (folder == nullptr)
			folder = "/tmp";
		return path(folder);
		#endif
	}
	case kLog:
	{
		char const* folder = getenv("XDG_CACHE_HOME");
		fs::temp_directory_path();
		if (folder == nullptr)
			return path(getenv("HOME")) / "/.cache";
		return path(folder);
	}
#endif
	}

#ifdef WIN32
	KNOWNFOLDERID id;

#define WINCASE(X) case kWin_##X: id = FOLDERID_##X; break;
	switch (relartiveTo)
	{
		WINCASE(RoamingAppData);
		WINCASE(LocalAppData);
		WINCASE(LocalAppDataLow);
		WINCASE(ProgramData);
		WINCASE(ProgramFiles);
		WINCASE(ProgramFilesX86);
		WINCASE(System);
		WINCASE(Windows);
		WINCASE(RecycleBinFolder);
		WINCASE(AccountPictures);
		WINCASE(ApplicationShortcuts);
		WINCASE(Documents);
		WINCASE(Downloads);
		WINCASE(UserProfiles);
		WINCASE(Fonts);
		WINCASE(InternetCache);
		WINCASE(Pictures);
		WINCASE(AdminTools);
		WINCASE(CDBurning);
		WINCASE(CommonAdminTools);
		WINCASE(CommonPrograms);
		WINCASE(CommonStartMenu);
		WINCASE(CommonStartup);
		WINCASE(CommonTemplates);
		WINCASE(Contacts);
		WINCASE(Cookies);
		WINCASE(Desktop);
		WINCASE(DeviceMetadataStore);
		WINCASE(DocumentsLibrary);
		WINCASE(History);
		WINCASE(Libraries);
		WINCASE(Links);
		WINCASE(LocalizedResourcesDir);
		WINCASE(Music);
		WINCASE(Videos);
		WINCASE(MusicLibrary);
		WINCASE(NetHood);
		WINCASE(PhotoAlbums);
		WINCASE(Profile);
		WINCASE(ProgramFilesCommon);
		WINCASE(ProgramFilesCommonX86);
		WINCASE(Public);
		WINCASE(PublicDesktop);
		WINCASE(PublicDocuments);
		WINCASE(PublicDownloads);
		WINCASE(PublicLibraries);
		WINCASE(PublicMusic);
		WINCASE(PublicPictures);
		WINCASE(PublicVideos);

	case kAbsolute:
	default:
		return "";
	}
#undef WINCASE

	LPWSTR wszPath = NULL;
	HRESULT result = SHGetKnownFolderPath(id, 0, NULL, &wszPath);
	if (result == S_OK)
	{
		path relativepath(wszPath);

		CoTaskMemFree(wszPath);
		return relativepath;
	}
#endif
#ifdef __linux__

#define getOrDefault(X, D)  const char* _d = getenv(#X); path folder; if (_d == nullptr) {folder = D;} else { folder = _d;}
	switch (relartiveTo)
	{
		case kStorageSynced:
		{
			getOrDefault(XDG_DATA_HOME, "/.local/share")
			return getenv("HOME") / folder;
		}
		case kStorageLocal:
		{
			getOrDefault(XDG_CACHE_HOME, "/.cache")
			return getenv("HOME") / folder;
		}
		case kDesktop:
		{
			getOrDefault(XDG_DESKTOP_DIR, "/Desktop")
			return getenv("HOME") / folder;
		}
		case kUserFiles:
		{
			getOrDefault(XDG_DOCUMENTS_DIR, "/Documents")
			return getenv("HOME") / folder;
		}
		case kMusic:
		{
			getOrDefault(XDG_MUSIC_DIR, "/Music")
			return getenv("HOME") / folder;
		}
		case kPictures:
		{
			getOrDefault(XDG_PICTURES_DIR, "/Pictures")
			return getenv("HOME") / folder;
		}


	case kAbsolute:
	default:
		return "";
	}
#endif

    return ""; // DZSIM_MOD: Added missing return statement
}

path Location::GetFullPath() const
{
	switch (m_relartiveTo)
	{
	case kAbsolute:
	{
		return m_filepath;
	}
	case kCurrentDirectory:
	case kSearchPathsAndArchives:
	{
		if (m_filepath.is_absolute())
		{
			return m_filepath;
		}
		return GetCurrentDirectory() / m_filepath;
	}
	default:
		return Location::GetSytemPath(m_relartiveTo) / m_filepath;
	}
}
