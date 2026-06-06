#include "DynamicAllocator.h"

#include "AllocatorUtils.h"

#include "Utils/BitMath.h"

namespace DynamicAllocator
{
	struct Header
	{
		size_t Size;
		size_t Padding;
	};

	static constexpr size_t GlobalAlignment = std::max(static_cast<size_t>(16), alignof(Header));

	void* Allocate(size_t size)
	{
		const size_t allocationSize = size + sizeof(Header) + 2 * GlobalAlignment;

		std::byte* const allocatedPtr = static_cast<std::byte*>(malloc(allocationSize + GlobalAlignment - 1));

		const AllocatorUtils::AddHeaderResult result = AllocatorUtils::AddHeader<Header>(allocatedPtr, GlobalAlignment);
		result.HeaderPtr->Size = size;
		result.HeaderPtr->Padding = result.Padding;

		const auto newSpan = AllocatorUtils::AddCheckBytes(result.UserPtr, size + 2 * GlobalAlignment, GlobalAlignment);
		ERP_VERIFY(newSpan.size() == size);

		return newSpan.data();
	}

	void Deallocate(void* ptr, size_t size)
	{
		if (!ptr)
			return;

		const auto newSpan = AllocatorUtils::RemoveCheckBytes(static_cast<std::byte*>(ptr), size, GlobalAlignment);

		const AllocatorUtils::RemoveHeaderResult result = AllocatorUtils::RemoveHeader<Header>(newSpan.data());
		ERP_VERIFY(size == result.HeaderPtr->Size);

		free(result.AllocatedPtr);
	}
}