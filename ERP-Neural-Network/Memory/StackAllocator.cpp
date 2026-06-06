#include "StackAllocator.h"

#include "AllocatorUtils.h"

#include "Utils/BitMath.h"

StackAllocator::StackAllocator()
	: m_Memory(nullptr), m_MemorySize(0), m_Ptr(nullptr), m_Head(nullptr) {}

void StackAllocator::SetMemoryBlock(void* ptr, size_t size)
{
	ERP_ASSERT(!m_Memory, "Tried to set new block again.");

	m_Memory = static_cast<std::byte*>(ptr);
	m_MemorySize = size;
	m_Ptr = m_Memory;
}

void StackAllocator::AllocateBlock(size_t size)
{
	ERP_ASSERT(!m_Memory, "Tried to allocate new block again.");

	m_Memory = static_cast<std::byte*>(::operator new(size));
	m_MemorySize = size;
	m_Ptr = m_Memory;
#if _DEBUG
	ERP_LOG("Allocated block for stack allocator of ", size, " bytes.");
#endif
}

void StackAllocator::DeallocateBlock()
{
	ERP_ASSERT(m_Ptr == m_Memory, "Stack allocator is not empty.");
	ERP_ASSERT(m_Memory, "Memory was nullptr.");

#if _DEBUG
	ERP_LOG("Deallocated block for stack allocator of ", m_MemorySize, " bytes. Peak usage : ", m_PeakUsage);
#endif
	::operator delete(m_Memory, m_MemorySize);
}

void StackAllocator::Release()
{
	m_Ptr = m_Memory;
	m_Head = nullptr;
}

void* StackAllocator::Allocate(size_t size, size_t alignment)
{
	ERP_ASSERT(size > 0, "Tried to allocate zero size.");
	ERP_ASSERT(BitMath::IsPowerOfTwo(alignment), "The alignment must be a power of two.");

	const auto result = AllocatorUtils::AddHeader<Header>(m_Ptr, alignment);

	std::byte* const nextPtr = result.UserPtr + size + 2 * alignment;

	ERP_VERIFY(nextPtr <= (m_Memory + m_MemorySize), "Stack allocator overflow.");

	const std::span<std::byte> newSpan = AllocatorUtils::AddCheckBytes(result.UserPtr, size + 2 * alignment, alignment);

	ERP_VERIFY(BitMath::IsAligned(newSpan.data(), alignment));
	ERP_VERIFY(newSpan.size() == size);

	result.HeaderPtr->Next = m_Head;
	result.HeaderPtr->Size = newSpan.size();
	result.HeaderPtr->Alignment = alignment;
	result.HeaderPtr->Padding = result.Padding;

	m_Ptr = nextPtr;
	m_Head = newSpan.data();
#if _DEBUG
	m_PeakUsage = std::max(m_PeakUsage, static_cast<size_t>(static_cast<std::byte*>(nextPtr) - m_Memory));
#endif

	return newSpan.data();
}

void StackAllocator::Deallocate(void* ptr, size_t size, size_t alignment)
{
	if (!ptr)
		return;

	ERP_VERIFY(ptr == m_Head, "Memory was deallocated in the wrong order.");

	const std::span<std::byte> newSpan = AllocatorUtils::RemoveCheckBytes(static_cast<std::byte*>(ptr), size, alignment);

	const auto result = AllocatorUtils::RemoveHeader<Header>(newSpan.data());

	ERP_VERIFY(size == result.HeaderPtr->Size);
	ERP_VERIFY(alignment == result.HeaderPtr->Alignment);
	ERP_VERIFY(((m_Ptr - alignment) - static_cast<std::byte*>(ptr)) == result.HeaderPtr->Size);

	m_Head = result.HeaderPtr->Next;

	m_Ptr = result.AllocatedPtr;
}