#pragma once
#include "fsal_common.h"
#include "FastPathNormalization.h"
#include <vector>
#include <string>
#include <algorithm>
#include <mutex>

namespace fsal
{
	template<typename UserData>
	struct FileEntry
	{
		FileEntry(): depth(0) {};
		FileEntry(const std::string& str) : depth(0), data(UserData())
		{
			NormalizePath(str, path, filenamePos, depth);
		}
		FileEntry(const std::string& str, const UserData& data) : depth(0), data(data)
		{
			NormalizePath(str, path, filenamePos, depth);
		}

		// Full path in archive.
		std::string path;

		// Name of the entry. Ends with slash for directories
		int filenamePos;

		// Depth in file tree. Files in the root dir have zero depth
		int depth;

		UserData data;
	};


	// Compare operator. Deeper path is always greater
	template<typename UserData>
	inline bool operator <(const FileEntry<UserData>& a, const FileEntry<UserData>& b)
	{
		if (a.depth == b.depth)
		{
			return a.path < b.path;
		}
		else
		{
			return a.depth < b.depth;
		}
	}

	inline int strcmpl(const char * __restrict l, const char * __restrict r, const char*& __restrict end)
	{
		for (; *l==*r && *l; ++l, ++r);
		end=r;
		return *(unsigned char *)l - *(unsigned char *)r;
	}

	template<typename UserData>
	class FileList
	{
	public:
		UserData FindEntry(const fs::path& path)
		{
            // DZSIM_MOD: Changed method from u8string() to string() for C++20 compatibility
			FileEntry<UserData> key(path.string());
			//FileEntry<UserData> key(path.u8string());

			int index = GetIndex(key).first;
			if (index != -1)
			{
				return m_fileList[index].data;
			}
			return UserData();
		}

		std::pair<int, int> GetIndex(const FileEntry<UserData>& key, bool getLowerBound = false)
		{
			if (!sorted)
			{
				std::lock_guard<std::mutex> lock(m_table_modification);
				std::sort(m_fileList.begin(), m_fileList.end());
				int depth = 0;
				depthTable.push_back(0);
				for (int i = 0, l = (int)m_fileList.size(); i != l; ++i)
				{
					if (depth != m_fileList[i].depth)
					{
						int newDepth = m_fileList[i].depth;
						depthTable.resize(newDepth + 1, depthTable[depth]);
						depthTable[newDepth] = i;
						depth = newDepth;
					}
				}
				depthTable.push_back((int)m_fileList.size());
				sorted = true;
			}

			if (key.depth + 1 >= (int)depthTable.size())
			{
				return std::make_pair(-1, -1);
			}

			size_t right = depthTable[key.depth + 1];
			size_t left = depthTable[key.depth];
			size_t it = 0;

			// Searching for lower bound
			size_t count = right - left;
			if (count == 0)
				return std::make_pair(left, right);

			const char* key_cstr = key.path.c_str();
			int start_compare_from = 0;
			int start_compare_from_l = 0;
			int start_compare_from_r = 0;

			auto* __restrict file_list = &m_fileList[0];
			const char* end = key_cstr;

			while (count > 0)
			{
				it = left;
				size_t step = count / 2;
				it += step;

				int res = strcmpl(file_list[it].path.c_str() + start_compare_from, key_cstr + start_compare_from, end);
				if (res == 0)
				{
					//printf("%s\n", file_list[it].path.c_str());
					return std::make_pair(it, it);
				}

				if (res < 0)
				{
					left = ++it;
					count -= step + 1;
					start_compare_from_l = end - key_cstr;
				}
				else
				{
					count = step;
					start_compare_from_r = end - key_cstr;
				}
				start_compare_from = std::min(start_compare_from_l, start_compare_from_r);
			}

			if (getLowerBound)
			{
				return std::make_pair(left, right);
			}

			return std::make_pair(-1, -1);
		}

		bool Exists(const FileEntry<UserData>& key)
		{
			auto index = GetIndex(key).first;
			return index != -1;
		}

		void Add(const UserData& data, const std::string& path)
		{
			FileEntry<UserData> entry(path, data);
			m_fileList.push_back(entry);
			sorted = false;
		}

		std::vector<std::string> ListDirectory(const fs::path& path)
		{
			std::vector<std::string> result;

            // DZSIM_MOD: Changed method from u8string() to string() for C++20 compatibility
			auto _index = GetIndex((path / " ").string(), true);
			//auto _index = GetIndex((path / " ").u8string(), true);
			int index = _index.first;
			int lastIndex = _index.second;

            // DZSIM_MOD: Changed method from u8string() to string() for C++20 compatibility
			std::string u8path = NormalizePath(path.string());
			//std::string u8path = NormalizePath(path.u8string());

			size_t key_size = u8path.size();

			// Starting from the obtained low bound, we are going to grab all paths that start with given path.
			while(index != -1 && index < lastIndex && m_fileList[index].path.compare(0, key_size, u8path) == 0)
			{
				const FileEntry<UserData>& entry = m_fileList[index];
				std::string filename(entry.path.begin() + entry.filenamePos, entry.path.end());
				result.push_back(filename);
				++index;
			}

			return result;
		}

        // DZSIM_MOD: Added getter to fix memory leak from outside this class
        const std::vector<FileEntry<UserData>>& GetInternalFileList() {
            return m_fileList;
        }

		std::mutex m_table_modification;

	private:
		std::vector<int> depthTable;
		std::vector<FileEntry<UserData>> m_fileList;
		bool sorted = false;
	};
}
