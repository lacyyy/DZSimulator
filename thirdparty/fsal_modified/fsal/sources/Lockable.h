#pragma once
#include "fsal_common.h"
#include <mutex>


namespace fsal
{
	class Lockable
	{
	public:
		virtual ~Lockable() = default;
		virtual std::mutex* GetMutex() const { return &m_mutex; };
	private:
		mutable std::mutex m_mutex;
	};
}
