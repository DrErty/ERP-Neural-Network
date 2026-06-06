#pragma once

#include "ERP.h"

#include <cstddef>
#include <cstdint>

#include "Utils/BitMath.h"

namespace AllocatorUtils
{
	static constexpr std::byte CheckByte = std::byte(0xcc);

	template<typename Header>
	struct AddHeaderResult
	{
		std::byte* UserPtr;
		Header* HeaderPtr;
		uint8_t Padding;
	};

	template<typename Header>
	struct RemoveHeaderResult
	{
		std::byte* AllocatedPtr;
		Header* HeaderPtr;
	};

	template<typename Header>
	static AddHeaderResult<Header> AddHeader(std::byte* allocatedPtr, size_t alignment)
	{
		ERP_VERIFY(BitMath::IsPowerOfTwo(alignment));

		std::byte* const userPtr = BitMath::Align(BitMath::Align(allocatedPtr, alignof(Header)) + sizeof(Header), alignment);
		Header* const headerPtr = reinterpret_cast<Header*>(userPtr) - 1;

		const size_t padding = userPtr - allocatedPtr;
		ERP_VERIFY(padding <= UINT8_MAX);

		ERP_VERIFY(BitMath::IsAligned(userPtr, alignment));

		return {
			.UserPtr = userPtr,
			.HeaderPtr = headerPtr,
			.Padding = static_cast<uint8_t>(padding)
		};
	}

	template<typename Header>
	static RemoveHeaderResult<Header> RemoveHeader(std::byte* userPtr)
	{
		Header* const headerPtr = reinterpret_cast<Header*>(userPtr) - 1;

		std::byte* const allocatedPtr = userPtr - headerPtr->Padding;
		return {
			.AllocatedPtr = allocatedPtr,
			.HeaderPtr = headerPtr
		};
	}

	static std::span<std::byte> AddCheckBytes(std::byte* inPtr, size_t size, size_t alignment)
	{
		std::byte* outPtr = inPtr + alignment;

		for (size_t i = 0; i < alignment; i++)
			inPtr[i] = CheckByte;

		std::byte* endPtr = inPtr + size - alignment;
		for (size_t i = 0; i < alignment; i++)
			endPtr[i] = CheckByte;

		return { outPtr, size - alignment * 2 };
	}

	static std::span<std::byte> RemoveCheckBytes(std::byte* inPtr, size_t size, size_t alignment)
	{
		std::byte* outPtr = inPtr - alignment;

		for (size_t i = 0; i < alignment; i++)
			ERP_VERIFY(outPtr[i] == CheckByte);

		std::byte* endPtr = inPtr + size;
		for (size_t i = 0; i < alignment; i++)
			ERP_VERIFY(endPtr[i] == CheckByte);

		return { outPtr, size + alignment * 2 };
	}
}