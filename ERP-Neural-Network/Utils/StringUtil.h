#pragma once

#include <cmath>
#include <cstring>
#include <string_view>

#include <concepts>
#include <type_traits>

#include "Memory/StaticBuffer.h"

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

	template<std::unsigned_integral T>
	StringBuilder& Concat(T value)
	{
		size_t digits = 1;

		T tempValue = value;
		while (tempValue >= 10)
		{
			tempValue /= 10;
			digits++;
		}

		VerifyFit(digits);

		tempValue = value;

		for (size_t i = 0; i < digits; i++)
		{
			m_Buffer[m_Index + digits - i - 1] = '0' + tempValue % 10;
			tempValue /= 10;
		}

		m_Index += digits;

		return *this;
	}

	template<std::signed_integral T>
	StringBuilder& Concat(T value)
	{
		if constexpr (std::is_same_v<T, char>)
		{
			VerifyFit(1);
			m_Buffer[m_Index] = value;
			m_Index++;

			return *this;
		}

		if (value < 0)
		{
			Concat('-');
			value = -value;
		}
		Concat(static_cast<std::make_unsigned<T>::type>(value));
		return *this;
	}

	StringBuilder& Concat(double value)
	{
		double whole;
		double fract = std::modf(value, &whole);

		if (value < 0.0)
		{
			Concat('-');
			whole = -whole;
		}

		Concat(static_cast<uint64_t>(whole));
		Concat('.');
		Concat(static_cast<uint64_t>(fract * 1000.0));

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
private:
	size_t GetMaxStringSize()
	{
		return m_Buffer.GetCount() - 1;
	}

	StaticBuffer<char>& m_Buffer;
	size_t m_Index = 0;
};