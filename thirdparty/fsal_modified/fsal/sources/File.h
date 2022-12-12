#pragma once
#include "fsal_common.h"
#include "Status.h"
#include "Location.h"

#include <memory>
#include <mutex>
#include <stddef.h>


namespace fsal
{
	class FileInterface;
	class File;

	class File
	{
	public:
		class LockGuard
		{
		public:
			explicit LockGuard(const FileInterface* file);
		private:
			std::lock_guard<std::mutex> guard;
		};

		enum Origin
		{
			Beginning,
			CurrentPosition,
			End
		};

		struct borrow {};

		File();

		File(FileInterface* file);

		File(FileInterface* file, borrow);

		operator bool() const;

		operator std::string() const;

		void operator =(const std::string& x);

		Status Read(uint8_t* destanation, size_t size, size_t* readBytes = nullptr) const;

		Status Write(const uint8_t* source, size_t size);

		Status Seek(ptrdiff_t offset, Origin origin = Beginning) const;

		size_t Tell() const;

		size_t GetSize() const;

		path GetPath() const;

		Status Flush() const;

        // DZSIM_MOD: Commented out this function for C++20 compatibility
		// uint64_t GetLastWriteTime() const;

		const uint8_t* GetDataPointer() const;

		uint8_t* GetDataPointer();

		template<typename T>
		Status Read(T& data)
		{
			return Read(reinterpret_cast<uint8_t*>(&data), sizeof(T));
		}

		template<typename T>
		Status Write(const T& data)
		{
			return Write(reinterpret_cast<const uint8_t*>(&data), sizeof(T));
		}

		std::shared_ptr<FileInterface> GetInterface() { return m_file;}
	private:
		std::shared_ptr<FileInterface> m_file;
	};
}
