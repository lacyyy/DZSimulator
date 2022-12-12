#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <fsal.h>
#include "doctest.h"


TEST_CASE("API_demo")
{
	fsal::FileSystem fs;
	fs.PushSearchPath("some_path/"); // Adding search path
	fs.PushSearchPath("../"); // Adding another search path

	fs.CreateDirectory("some_path"); // Creating directory
	fs.Open("some_path/test.txt", fsal::kWrite) = std::string("test content"); // Writing std::string to a file

	std::string content = fs.Open("test.txt"); // Reading file to std::string (we can open it because of the search path)
	CHECK(content == "test content");

	auto file = fs.Open("somefile.bin"); // Open file for reading

	size_t size = file.GetSize(); // Getting size
	CHECK(size == 128);
	fsal::path path = file.GetPath(); // Get path of the file
	CHECK(path.filename() == "somefile.bin");

	file.Seek(110, fsal::File::Beginning); // Seek file

	uint32_t some_int;
	file.Read(some_int); // Reading 4-byte unsigned int

	int16_t some_short;
	file.Read(some_short); // Reading 2-byte signed int

	uint8_t buff[200];
	auto status = file.Read(buff, 10); // Reading 10 bytes to buff
	bool is_eof = status.is_eof(); // is EOF?
	CHECK(status.ok());
	CHECK(!is_eof);

	status = file.Read(some_short); // Reading one more time 2-byte signed int to reach end (no EOF yet, it's generated only when reading beyond end of file)
	CHECK(status.ok());
	CHECK(!status.is_eof());

	status = file.Read(some_short); // Reading again and getting EOF
	CHECK(status.ok());
	CHECK(status.is_eof());
}

TEST_CASE("Filepath normalization")
{
		CHECK(fsal::NormalizePath("./test_folder/folder_inside/../folder_inside/./") == "test_folder/folder_inside/");
		CHECK(fsal::NormalizePath("test_folder/folder_inside/../folder_inside/") == "test_folder/folder_inside/");
		CHECK(fsal::NormalizePath("test_folder/../test_folder/./folder_inside/../folder_inside/") == "test_folder/folder_inside/");
		CHECK(fsal::NormalizePath("test_folder/folder_inside/../folder_inside/.") == "test_folder/folder_inside");
}

TEST_CASE("Filepaths")
{
	using fs = fsal::FileSystem;
	using loc = fsal::Location;
	printf("RoamingAppData: %s\n", fs::GetSystemPath(loc::kStorageSynced).string().c_str());
	printf("LocalAppData: %s\n", fs::GetSystemPath(loc::kStorageLocal).string().c_str());
	printf("UserPictures: %s\n", fs::GetSystemPath(loc::kPictures).string().c_str());
	printf("UserMusic: %s\n", fs::GetSystemPath(loc::kMusic).string().c_str());
	printf("ProgramData: %s\n", fs::GetSystemPath(loc::kWin_ProgramData).string().c_str());
	printf("ProgramFiles: %s\n", fs::GetSystemPath(loc::kWin_ProgramFiles).string().c_str());
	printf("Temp: %s\n", fs::GetSystemPath(loc::kTemp).string().c_str());
}

TEST_CASE("ReadWriteToFile")
{
	fsal::FileSystem fs;
	fs.PushSearchPath("../");

	{
		fsal::File f = fs.Open(fsal::Location("test.txt", fsal::Location::kCurrentDirectory), fsal::kWrite);
				CHECK(f);
		f = std::string("asdasdasd");
	}
	{
		fsal::File f = fs.Open(fsal::Location("test.txt", fsal::Location::kCurrentDirectory), fsal::kRead);
				CHECK(f);
		std::string str = f;
				CHECK(str == std::string("asdasdasd"));
	}
}

TEST_CASE("OpenZIP")
{
	fsal::FileSystem fs;
	fs.PushSearchPath("../");
	{
		fsal::ZipReader zip;
		auto zipfile = fs.Open("test_archive.zip");
		CHECK(zipfile);

		fsal::Status r = zip.OpenArchive(zipfile);
		CHECK(r.ok());

		auto file = zip.OpenFile("test_folder/folder_inside/../folder_inside/./test_file.txt");
		CHECK(file);

		std::string str = file;
		CHECK(str == "test");

		auto l1 = zip.ListDirectory("./test_folder/folder_inside/../folder_inside/./");
		printf("\nListDirectory ./test_folder/folder_inside/../folder_inside/./\n");
		for (const auto& l: l1)
		{
			printf("%s\n", l.c_str());
		}
		auto l2 = zip.ListDirectory(".");
		printf("\nListDirectory .\n");
		for (const auto& l: l2)
		{
			printf("%s\n", l.c_str());
		}
	}
}

TEST_CASE("CreateZIP")
{
	fsal::FileSystem fs;
	fs.PushSearchPath("../");
	{
		auto zipfile = fs.Open("out_archive.zip", fsal::kWrite);
		fsal::ZipWriter zip(zipfile);
		CHECK(zipfile);

		zip.AddFile("CMakeLists.txt", fs.Open("CMakeLists.txt"));
		zip.CreateDirectory("tests");
		zip.AddFile("tests/main.cpp", fs.Open("tests/main.cpp"));
		zip.CreateDirectory("tests2");
	}
}

TEST_CASE("CreateZIP_LZ4")
{
	fsal::FileSystem fs;
	fs.PushSearchPath("../");
	std::string original = fs.Open("CMakeLists.txt");
	{
		auto zipfile = fs.Open("out_archive_lz4.zip", fsal::kWrite);
		fsal::ZipWriter zip(zipfile);
		CHECK(zipfile);

		zip.AddFile("CMakeLists.txt", fs.Open("CMakeLists.txt"), fsal::ZIP_COMPRESSION::LZ4);
		zip.CreateDirectory("tests");
		zip.AddFile("tests/main.cpp", fs.Open("tests/main.cpp"), fsal::ZIP_COMPRESSION::LZ4);
		zip.CreateDirectory("tests2");
	}
	{
		auto zipfile = fs.Open("out_archive_lz4.zip");
		fsal::ZipReader zip;
		zip.OpenArchive(zipfile);
		CHECK(zipfile);

		auto file = zip.OpenFile("CMakeLists.txt");
		std::string decompressed = file;

		CHECK(original == decompressed);
	}
}

TEST_CASE("MountZIP")
{
	fsal::FileSystem fs;
	fs.PushSearchPath("../");
	{
		auto zipfile = fs.Open("test_archive.zip", fsal::kRead, true);
		CHECK(zipfile);

		auto zip = fsal::OpenZipArchive(zipfile);
		fs.MountArchive(zip);

		fsal::File file = fs.Open("test_folder/folder_inside/test_file.txt", fsal::kRead);
		CHECK(file);
		std::string str = file;
		CHECK(str == "test");

		file = fs.Open("test_folder/folder_inside/123.png", fsal::kRead);
		std::string content = file;
		fs.Open("123.png", fsal::Mode::kWrite) = content;

		printf("\nListDirectory test_folder/folder_inside\"\n");
		auto list = zip.ListDirectory("test_folder/folder_inside");
		for(auto s: list)
		{
			printf("%s\n", s.c_str());
		}
	}
}

TEST_CASE("MountUncompressedZIP")
{
	fsal::FileSystem fs;
	fs.PushSearchPath("../");
	{
		auto zipfile = fs.Open("zero_compression.zip", fsal::kRead, true);
		CHECK(zipfile);

		auto zip = fsal::OpenZipArchive(zipfile);
		fs.MountArchive(zip);

		fsal::File file = fs.Open("123/2.png", fsal::kRead);
		size_t size = file.GetSize();

		std::string content;
		content.resize(size);

		int i = 0;
		for (i = 0; i < int(size / 1024); ++i)
		{
			file.Read((uint8_t*)&content[0] + i * 1024, 1024, nullptr);
		}

		file.Read((uint8_t*)&content[0] + i * 1024,  size - i * 1024, nullptr);

		fs.Open("2.png", fsal::Mode::kWrite) = content;

		auto list = zip.ListDirectory(".");
		printf("\nListDirectory .\n");
		for(auto s: list)
		{
			printf("%s\n", s.c_str());
		}
	}
}

TEST_CASE("MountVpk" * doctest::skip())
{
	printf("\nVPK\n");
	fsal::FileSystem fs;
	fs.PushSearchPath("../");
	{
		auto archive = fsal::OpenVpkArchive(fs, "/mnt/StorageExt4/SteamLibrary/steamapps/common/Counter-Strike Global Offensive/csgo");
		fs.MountArchive(archive);
		fsal::File file = fs.Open("materials/panorama/images/map_icons/map_icon_de_dust2.svg", fsal::kRead);
		std::string content = file;
		fs.Open("map_icon_de_dust2.svg", fsal::Mode::kWrite) = content;

		auto list = archive.ListDirectory(".");
		printf("\nListDirectory .\n");
		for(auto s: list)
		{
			printf("%s\n", s.c_str());
		}
		list = archive.ListDirectory("models");
		printf("\nListDirectory models\n");
		for(auto s: list)
		{
			printf("%s\n", s.c_str());
		}
	}
}
