#pragma once

#include "ERP.h"

#include "Iterator.h"
#include "StackAllocator.h"

template<typename T, size_t C>
class StackVector
{
public:
	StackVector()
		: m_Count(0)
	{
		m_Data = static_cast<T*>(Memory::ThreadStackAllocator.Allocate(C * sizeof(T), alignof(T)));
	}

	StackVector(const StackVector&) = delete;
	StackVector& operator=(const StackVector&) = delete;
	StackVector(StackVector&& other) noexcept
		: m_Count(other.m_Count), m_Data(other.m_Data)
	{
		other.m_Data = nullptr;
		ERP_VERIFY(false);
	}

	StackVector& operator=(StackVector&&) = delete;

	~StackVector()
	{
		for (size_t i = m_Count; i >= 1; i--)
			Get(i - 1).~T();

		Memory::ThreadStackAllocator.Deallocate(m_Data, C * sizeof(T), alignof(T));
	}

	Iterator<T> begin() { return Iterator<T>(m_Data); }
	Iterator<T> end() { return Iterator<T>(m_Data + m_Count); }

	Iterator<const T> begin() const { return Iterator<const T>(m_Data); }
	Iterator<const T> end() const { return Iterator<const T>(m_Data + m_Count); }

	constexpr size_t GetMaxCount() const { return C; }
	size_t GetCount() const { return m_Count; }

	T* GetData() { return m_Data; }
	const T* GetData() const { return m_Data; }

	T& operator[](size_t index) { return Get(index); }
	const T& operator[](size_t index) const { return Get(index); }

	template<typename... Args> requires std::is_trivially_copyable_v<T>
	T& Add(size_t index, Args&&... args)
	{
		ERP_ASSERT(index <= m_Count, "Index out of range");
		ERP_VERIFY(m_Count < C, "Static Array full.");

		m_Count++;
		const size_t moveCount = m_Count - index - 1;
		if (moveCount > 0)
		{
			memmove(&Get(index + 1), &Get(index), moveCount * sizeof(T));
		}
		new(&Get(index)) T(std::forward<Args>(args)...);
		return Get(index);
	}

	template<typename... Args>
	T& AddEnd(Args&&... args)
	{
		ERP_VERIFY(m_Count < C, "Stack Vector full.");
		new(&Get(m_Count++)) T(std::forward<Args>(args)...);
		return Get(m_Count - 1);
	}

	void Remove(size_t index)
	{
		static_assert(std::is_trivially_copyable_v<T>);

		ERP_ASSERT(index < m_Count, "Index out of range");

		Get(index).~T();
		const size_t moveCount = m_Count - index - 1;
		if (moveCount > 0)
		{
			memmove(&Get(index), &Get(index + 1), moveCount * sizeof(T));
		}
		m_Count--;
	}

	// Faster than normal remove.
	void RemoveEnd()
	{
		ERP_ASSERT(m_Count > 0, "Array is empty");
		Get(--m_Count).~T();
	}

	void RemoveAll()
	{
		for (size_t i = m_Count; i >= 1; i--)
			Get(i - 1).~T();

		m_Count = 0;
	}
private:
	T& Get(size_t index)
	{
		ERP_ASSERT(index < m_Count, "Index out of range");
		return m_Data[index];
	}

	const T& Get(size_t index) const
	{
		ERP_ASSERT(index < m_Count, "Index out of range");
		return m_Data[index];
	}
private:
	size_t m_Count;
	T* m_Data;
};