#pragma once
#include "fsal_common.h"
#include "Status.h"
#include "Location.h"
#include "File.h"
#include "Archive.h"

namespace fsal
{
	struct FsalImplementation;

	class FileSystem
	{
	public:
		FileSystem();

		File Open(const Location& location, Mode mode = kRead, bool lockable = false);

		bool Exists(const Location& location);

		Status Rename(const Location& srcLocation, const Location& dstLocation);

		Status Remove(const Location& location);

		Status CreateDirectory(const Location& location);

		void PushSearchPath(const Location& location);

		void PopSearchPath();

		void ClearSearchPaths();

		Status MountArchive(const Archive& archive);

        void UnmountAllArchives();

		static path GetSystemPath(const Location::Options& options);

	private:
		Status Find(const Location& location, path& absolutePath, PathType& type, Archive& archive);
		std::shared_ptr<FsalImplementation> m_impl;
	};
}
