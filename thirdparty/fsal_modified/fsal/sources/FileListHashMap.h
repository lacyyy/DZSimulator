#pragma once
#include "fsal_common.h"
#include "FastPathNormalization.h"
#include <vector>
#include <string>
#include <algorithm>
#include <mutex>
#include <unordered_map>

namespace fsal
{
	template<typename UserData>
	struct FileEntry
	{
		FileEntry() {};
		FileEntry(const std::string& str) : data(UserData())
		{
			int depth;
			NormalizePath(str, path, filenamePos, depth);
		}
		FileEntry(const std::string& str, const UserData& data) : data(data)
		{
			int depth;
			NormalizePath(str, path, filenamePos, depth);
		}

		// Full path in archive.
		std::string path;

		// Name of the entry. Ends with slash for directories
		int filenamePos;

		UserData data;
	};

	template<typename UserData>
	class FileList
	{
	public:
		UserData FindEntry(const fs::path& path)
		{
			FileEntry<UserData> key(path.u8string());

			auto it = m_filemap.find(key.path);
			if (it != m_filemap.end())
			{
				return it->second;
			}
			return UserData();
		}

		void Add(const UserData& data, const std::string& path)
		{
			FileEntry<UserData> entry(path, data);
			m_filemap[entry.path] = data;
		}

		bool Exists(const FileEntry<UserData>& key)
		{
			auto it = m_filemap.find(key.path);
			return it != m_filemap.end();
		}

		std::vector<std::string> ListDirectory(const fs::path& path)
		{
//			std::vector<std::string> result;
//
//			int index = GetIndex((path / "a").string(), true);
//
//			int lastIndex = m_fileList[index].depth + 1 < (int)depthTable.size() ? depthTable[m_fileList[index].depth + 1] : (int)m_fileList.size();
//
//			std::string u8path = path.u8string();
//
//			size_t key_size = u8path.size();
//
//			// Starting from the obtained low bound, we are going to grab all paths that start with given path.
//			while(index != -1 && index < lastIndex && m_fileList[index].path.compare(0, key_size, u8path) == 0)
//			{
//				const FileEntry<UserData>& entry = m_fileList[index];
//				std::string filename(entry.path.begin() + entry.filenamePos, entry.path.end());
//				result.push_back(filename);
//				++index;
//			}
//
//			return result;
		}

		std::mutex m_table_modification;

	private:
		std::unordered_map<std::string, UserData> m_filemap;
	};
}
