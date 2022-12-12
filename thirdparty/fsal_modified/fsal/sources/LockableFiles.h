#pragma once
#include "StdFile.h"
#include "MemRefFile.h"
#include "Lockable.h"


namespace fsal
{
	class LStdFile : public StdFile, public Lockable
	{
	public:
		std::mutex* GetMutex() const override { return Lockable::GetMutex(); };
	};

	class LMemRefFile : public MemRefFile, public Lockable
	{
	public:
		std::mutex* GetMutex() const override { return Lockable::GetMutex(); };
	};
}
