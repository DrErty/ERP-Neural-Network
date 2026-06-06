#pragma once

namespace DynamicAllocator
{
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);
}