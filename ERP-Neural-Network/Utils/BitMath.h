#pragma once

#include <concepts>

namespace BitMath
{
	template<std::integral T>
	static constexpr T Align(T size, T alignment)
	{
		return (size + alignment - 1) & ~(alignment - 1);
	}

	template<typename T, std::integral U>
	static constexpr T* Align(T* ptr, U alignment)
	{
		return reinterpret_cast<T*>((reinterpret_cast<uintptr_t>(ptr) + alignment - 1) & ~(alignment - 1));
	}

	template<typename T, std::integral U>
	static constexpr bool IsAligned(T* ptr, U alignment)
	{
		return ptr == Align(ptr, alignment);
	}

	template<std::integral T>
	static constexpr bool IsPowerOfTwo(T n)
	{
		return (n > 0) && ((n & (n - 1)) == 0);
	}
}