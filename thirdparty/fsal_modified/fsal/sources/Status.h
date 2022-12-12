#pragma once
#include "fsal_common.h"

namespace fsal
{
	struct Status
	{
		enum State : uint8_t
		{
			kOk = 0,
			kEOF = 1u << 0u,
			kFailed = 1u << 1u
		};

		bool ok() const
		{
			return !(state & kFailed);
		}

		bool is_eof() const
		{
			return state & kEOF;
		}

		Status() :state(kFailed)
		{
		}

		Status(State state) :state(state)
		{
		}

		Status(bool state) :state(state ? State(0) : kFailed)
		{
		}
		
		operator bool() const
		{
			return ok();
		}
		
		State state;
	};

	inline Status::State operator | (Status::State a, Status::State b)
	{
		return (Status::State)(uint8_t(a) | uint8_t(b));
	}

	inline Status::State operator |= (Status::State& a, Status::State b)
	{
		a = a | b;
		return a;
	}

	inline Status::State operator & (Status::State a, Status::State b)
	{
		return (Status::State)(uint8_t(a) & uint8_t(b));
	}

	inline Status::State operator &= (Status::State& a, Status::State b)
	{
		a = a & b;
		return a;
	}
}
