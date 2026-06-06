#pragma once

#include <cstddef>
#include <cstdint>

class StackAllocator
{
public:
	StackAllocator();

	StackAllocator(const StackAllocator&) = delete;
	StackAllocator& operator=(const StackAllocator&) = delete;
	StackAllocator(StackAllocator&&) = delete;
	StackAllocator& operator=(StackAllocator&&) = delete;

	void* GetMemoryBlock() { return m_Memory; }

	void SetMemoryBlock(void* ptr, size_t size);

	void AllocateBlock(size_t size);
	void DeallocateBlock();

	void Release();

	void* Allocate(size_t size, size_t alignment);
	void Deallocate(void* ptr, size_t size, size_t alignment);
private:
	struct Header
	{
		std::byte* Next;
		size_t Size;
		size_t Alignment;
		uint8_t Padding;
	};
private:
	std::byte* m_Memory;
	size_t m_MemorySize;

	std::byte* m_Ptr;
	std::byte* m_Head;
#if _DEBUG
	size_t m_PeakUsage = 0;
#endif
};

namespace Memory
{
	inline thread_local StackAllocator ThreadStackAllocator;
}