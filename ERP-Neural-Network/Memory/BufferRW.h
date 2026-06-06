#pragma once

#include "ERP.h"

class BufferRW
{
public:
	BufferRW(std::span<char> buffer)
		: m_Buffer(buffer), m_Cursor(0)
	{

	}

	void WriteU32(uint32_t value)
	{
		WriteU8((value & 0x00'00'00'ff));
		WriteU8((value & 0x00'00'ff'00) >> 8);
		WriteU8((value & 0x00'ff'00'00) >> 16);
		WriteU8((value & 0xff'00'00'00) >> 24);
	}

	uint32_t ReadU32()
	{
		uint8_t byte1 = ReadU8();
		uint8_t byte2 = ReadU8();
		uint8_t byte3 = ReadU8();
		uint8_t byte4 = ReadU8();

		return (byte4 << 24) | (byte3 << 16) | (byte2 << 8) | byte1;
	}

	void WriteU8(uint8_t value)
	{
		static_assert(sizeof(uint8_t) == 1);

		ERP_VERIFY((m_Cursor + sizeof(uint8_t)) <= m_Buffer.size());
		*(m_Buffer.data() + m_Cursor) = value;
		m_Cursor += sizeof(uint8_t);
	}

	uint8_t ReadU8()
	{
		static_assert(sizeof(uint8_t) == 1);

		ERP_VERIFY((m_Cursor + sizeof(uint8_t)) <= m_Buffer.size());
		uint8_t value = *(m_Buffer.data() + m_Cursor);
		m_Cursor += sizeof(uint8_t);
		return value;
	}

	void WriteChar(char value)
	{
		ERP_VERIFY((m_Cursor + sizeof(char)) <= m_Buffer.size());
		*(m_Buffer.data() + m_Cursor) = value;
		m_Cursor += sizeof(char);
	}

	char ReadChar()
	{
		ERP_VERIFY((m_Cursor + sizeof(char)) <= m_Buffer.size());
		char value = *(m_Buffer.data() + m_Cursor);
		m_Cursor += sizeof(char);
		return value;
	}
private:
	std::span<char> m_Buffer;
	size_t m_Cursor;
};