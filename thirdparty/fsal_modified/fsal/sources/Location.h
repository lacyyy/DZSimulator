#pragma once
#include "fsal_common.h"

#include <string>

namespace fsal
{
	struct Location
	{
		friend class FileSystem;

		enum Options : uint16_t
		{
			kAbsolute = 0x000,					// The path will be treated as an absolute. If the provided path is not absolute, then it will result in failure.
			kCurrentDirectory,					// The path will be treated as a relative to the current directory
			kSearchPaths,						// The path will be treated as a relative to all current searchpaths. 
			kArchives,							// The path will be treated as a path in all mounted archives
			kSearchPathsAndArchives,			// The path will be treated as a relative to all current searchpaths. 

			// Universal Section.
			kUserFiles,							// kNS_DocumentDirectory kWin_Documents
			kStorageSynced,						// kNS_LibraryDirectory kWin_RoamingAppData
			kStorageLocal,						// kNS_CachesDirectory kWin_LocalAppData
			kUsers,								// kNS_UserDirectory kWin_UserProfiles
			kDesktop,							// kNS_DesktopDirectory kWin_Desktop
			kMusic,								// kNS_MusicDirectory kWin_Music 
			kPictures,							// kNS_PicturesDirectory kWin_Pictures 
			kPublic,							// kNS_SharedPublicDirectory kWin_Public
			kDownloads,							// kNS_DownloadsDirectory kWin_Downloads
			kTrashDirectory,					// kNS_TrashDirectory kWin_RecycleBinFolder
			kLog,								// Log directory
			kTemp,								// Temprorary directory

			// Apple NeXTSTEP
			kNS_DocumentDirectory = kUserFiles,     // Document directory.
			kNS_LibraryDirectory = kStorageSynced,	// Various user-visible documentation, support, and configuration files (/Library).
			kNS_CachesDirectory = kStorageLocal,    // Location of discardable cache files (Library/Caches).
			kNS_UserDirectory = kUsers,			    // User home directories (/Users).
			kNS_DesktopDirectory = kDesktop, 	    // Location of user�s desktop directory.
			kNS_MusicDirectory = kMusic,		    // Location of user's Music directory (~/Music)
			kNS_PicturesDirectory = kPictures,	    // Location of user's Pictures directory (~/Pictures)
			kNS_SharedPublicDirectory = kPublic,    // Location of user's Public sharing directory (~/Public)
			kNS_DownloadsDirectory = kDownloads,    // Location of the user�s downloads directory.
			kNS_TrashDirectory = kTrashDirectory,

			kNS_ApplicationDirectory = 0x100,	    // Supported applications (/Applications).
			kNS_DemoApplicationDirectory, 		    // Unsupported applications and demonstration versions.
			kNS_DeveloperApplicationDirectory, 	    // Developer applications (/Developer/Applications).
			kNS_AdminApplicationDirectory, 		    // System and network administration applications.
			kNS_DeveloperDirectory, 			    // Developer resources (/Developer).
			kNS_DocumentationDirectory, 		    // Documentation.
			kNS_CoreServiceDirectory, 			    // Location of core services (System/Library/CoreServices).
			kNS_AutosavedInformationDirectory, 	    // Location of user�s autosaved documents Library/Autosave Information
			kNS_ApplicationSupportDirectory, 	    // Location of application support files (Library/Application Support).
			kNS_InputMethodsDirectory, 			    // Location of Input Methods (Library/Input Methods)
			kNS_MoviesDirectory, 				    // Location of user's Movies directory (~/Movies)
			kNS_PrinterDescriptionDirectory, 	    // Location of system's PPDs directory (Library/Printers/PPDs)
			kNS_PreferencePanesDirectory, 		    // Location of the PreferencePanes directory for use with System Preferences (Library/PreferencePanes)
			kNS_ApplicationScriptsDirectory, 	    // Location of the user scripts folder for the calling application (~/Library/Application Scripts/<code-signing-id>
			kNS_ItemReplacementDirectory, 		    // Passed to the NSFileManager method URLForDirectory:inDomain:appropriateForURL:create:error: in order to create a temporary directory.
			kNS_AllApplicationsDirectory, 		    // All directories where applications can occur.
			kNS_AllLibrariesDirectory, 			    // All directories where resources can occur.

			// Windows section
			kWin_Documents = kUserFiles,
			kWin_RoamingAppData = kStorageSynced,
			kWin_LocalAppData = kStorageLocal,
			kWin_UserProfiles = kUsers,
			kWin_Desktop = kDesktop,
			kWin_Music = kMusic,
			kWin_Pictures = kPictures,
			kWin_Public = kPublic,
			kWin_Downloads = kDownloads,
			kWin_RecycleBinFolder = kTrashDirectory,

			kWin_LocalAppDataLow = 0x200,
			kWin_ProgramData,
			kWin_ProgramFiles,
			kWin_ProgramFilesX86,
			kWin_System,
			kWin_Windows,
			kWin_AccountPictures,
			kWin_ApplicationShortcuts,
			kWin_Fonts,
			kWin_InternetCache,
			kWin_AdminTools,
			kWin_CDBurning,
			kWin_CommonAdminTools,
			kWin_CommonPrograms,
			kWin_CommonStartMenu,
			kWin_CommonStartup,
			kWin_CommonTemplates,
			kWin_Contacts,
			kWin_Cookies,
			kWin_DeviceMetadataStore,
			kWin_DocumentsLibrary,
			kWin_History,
			kWin_Libraries,
			kWin_Links,
			kWin_LocalizedResourcesDir,
			kWin_Videos,
			kWin_MusicLibrary,
			kWin_NetHood,
			kWin_PhotoAlbums,
			kWin_Profile,
			kWin_ProgramFilesCommon,
			kWin_ProgramFilesCommonX86,
			kWin_PublicDesktop,
			kWin_PublicDocuments,
			kWin_PublicDownloads,
			kWin_PublicLibraries,
			kWin_PublicMusic,
			kWin_PublicPictures,
			kWin_PublicVideos,

			// Linux Section

		};

		Location(const char* filepath, Options location = kSearchPathsAndArchives, PathType type = kFile | kDirectory, LinkType link = kNotSymlink | kSymlink);

		Location(const std::string& filepath, Options location = kSearchPathsAndArchives, PathType type = kFile | kDirectory, LinkType link = kNotSymlink | kSymlink);

		Location(const path& filepath, Options location = kSearchPathsAndArchives, PathType type = kFile | kDirectory, LinkType link = kNotSymlink | kSymlink);
		
		Location(Options location);

		Location() = default;

		path GetFullPath() const;

		static path GetCurrentDirectory();

		static path GetSytemPath(Location::Options relartiveTo);

		Location operator / (const path& p)
		{
			Location other(*this);
			other.m_filepath = m_filepath / p;
			return other;
		}

	private:
		path m_filepath;
		Options m_relartiveTo;
		PathType m_type;
		LinkType m_link;
	};
}
