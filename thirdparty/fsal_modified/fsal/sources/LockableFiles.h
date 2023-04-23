#pragma once
#include "StdFile.h"
#include "MemRefFile.h"
#include "MemRefFileReadOnly.h" // DZSIM_MOD: Added new include
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

    // DZSIM_MOD:
    //   Added a lockable version of MemRefFileReadOnly. The lockable version is
    //   required to use SubFile with it (Even if lockability is technically not
    //   necessary for MemRefFileReadOnly, we just stick with the system...)
    class LMemRefFileReadOnly : public MemRefFileReadOnly, public Lockable
    {
    public:
        LMemRefFileReadOnly(const uint8_t* data, size_t size)
            : MemRefFileReadOnly{ data, size } {}
        std::mutex* GetMutex() const override { return Lockable::GetMutex(); };
    };
}
