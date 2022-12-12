# File System Abstraction Layer
[![Build Status](https://travis-ci.org/podgorskiy/fsal.svg?branch=master)](https://travis-ci.org/github/podgorskiy/fsal)


![Banner](https://repository-images.githubusercontent.com/141553188/8d423700-7ace-11ea-97a3-d1ab9deb0700)

FSAL is a C++ library that provides an abstract layer over a filesystem.

 * Standard API for file acces (read, write, tell, seek, etc.)
 * Access to last modification time
 * UTF8 for filenames on all platforms
 * Mounting ZIP and VPK archives (content is accessible in read-only mode as if they were unpacked)
 * In-memory files-like objects. Can be mutable, immutable, growable, and views (no-copy from user data pointer)
  
## Examples 
 
### Basic file API
```cpp
	fsal::FileSystem fs;
	fs.PushSearchPath("some_path/"); // Adding search path
	fs.PushSearchPath("../"); // Adding another search path

	fs.CreateDirectory("some_path"); // Creating directory
  
	// Writing std::string to a file
	fs.Open("some_path/test.txt", fsal::kWrite) = std::string("test content"); 

	// Reading file to std::string (we can open it because of the search path)
	std::string content = fs.Open("test.txt"); 
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

	// Reading one more time 2-byte signed int to reach end
	// (no EOF yet, it's generated only when reading beyond end of file)
	status = file.Read(some_short); 
	CHECK(status.ok());
	CHECK(!status.is_eof());

	status = file.Read(some_short); // Reading again and getting EOF
	CHECK(status.ok());
	CHECK(status.is_eof());
```

### Path normalization:
```cpp
	CHECK(fsal::NormalizePath("./test_folder/folder_inside/../folder_inside/./") == "test_folder/folder_inside/");
	CHECK(fsal::NormalizePath("test_folder/folder_inside/../folder_inside/") == "test_folder/folder_inside/");
	CHECK(fsal::NormalizePath("test_folder/../test_folder/./folder_inside/../folder_inside/") == "test_folder/folder_inside/");
	CHECK(fsal::NormalizePath("test_folder/folder_inside/../folder_inside/.") == "test_folder/folder_inside");
```

### Getting system locations:
```cpp
	using fs = fsal::FileSystem;
	using loc = fsal::Location;
	printf("RoamingAppData: %s\n", fs::GetSystemPath(loc::kStorageSynced).string().c_str());
	printf("LocalAppData: %s\n", fs::GetSystemPath(loc::kStorageLocal).string().c_str());
	printf("UserPictures: %s\n", fs::GetSystemPath(loc::kPictures).string().c_str());
	printf("UserMusic: %s\n", fs::GetSystemPath(loc::kMusic).string().c_str());
	printf("ProgramData: %s\n", fs::GetSystemPath(loc::kWin_ProgramData).string().c_str());
	printf("ProgramFiles: %s\n", fs::GetSystemPath(loc::kWin_ProgramFiles).string().c_str());
	printf("Temp: %s\n", fs::GetSystemPath(loc::kTemp).string().c_str());
```

### Reading ZIP archive:
```cpp
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
```

### Creating ZIP archive:
```cpp
	auto zipfile = fs.Open("out_archive.zip", fsal::kWrite);
	fsal::ZipWriter zip(zipfile);
	CHECK(zipfile);

	zip.AddFile("CMakeLists.txt", fs.Open("CMakeLists.txt"));
	zip.CreateDirectory("tests");
	zip.AddFile("tests/main.cpp", fs.Open("tests/main.cpp"));
	zip.CreateDirectory("tests2");
```

### Mounting ZIP archive:
```cpp
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
```

### Mounting VPK archive:
```cpp
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
```
  
