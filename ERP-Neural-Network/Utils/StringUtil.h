#pragma once

#include <cmath>
#include <cstring>
#include <string_view>

#include <concepts>
#include <type_traits>

#include <charconv>
#include <system_error>

#include "Memory/StaticBuffer.h"

template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

class StringBuilder
{
public:
	StringBuilder(StaticBuffer<char>& buffer)
		: m_Buffer(buffer) {}

	StringBuilder(const StringBuilder&) = delete;
	StringBuilder& operator=(const StringBuilder&) = delete;
	StringBuilder(StringBuilder&&) = delete;
	StringBuilder& operator=(StringBuilder&&) = delete;

	StringBuilder& Concat(std::string_view str)
	{
		VerifyFit(str.length());
		memcpy(m_Buffer.GetData() + m_Index, str.data(), str.length());
		m_Index += str.length();

		return *this;
	}

	template<Arithmetic T>
	StringBuilder& Concat(T value)
	{
		if constexpr (std::is_same_v<T, char>)
		{
			VerifyFit(1);
			m_Buffer[m_Index] = value;
			m_Index++;

			return *this;
		}

		auto result = std::to_chars(m_Buffer.GetData() + m_Index, m_Buffer.GetData() + m_Buffer.GetCount(), value);

		Verify(result.ec == std::errc{});

		m_Index = (result.ptr - m_Buffer.GetData());

		return *this;
	}

	StringBuilder& Skip(size_t count)
	{
		VerifyFit(count);
		m_Index += count;
		return *this;
	}

	bool CanFit(size_t length)
	{
		return (m_Index + length) <= GetMaxStringSize();
	}

	void Compile()
	{
		m_Buffer[m_Index] = '\0';
	}

	void VerifyFit(size_t length);
	void Verify(bool condition);
private:
	size_t GetMaxStringSize()
	{
		return m_Buffer.GetCount() - 1;
	}

	StaticBuffer<char>& m_Buffer;
	size_t m_Index = 0;
};