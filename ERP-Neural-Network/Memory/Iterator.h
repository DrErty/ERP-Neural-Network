#pragma once

template<typename T>
class Iterator
{
public:
	explicit Iterator(T* ptr)
		: m_Ptr(ptr) {}

	T& operator*() const { return *m_Ptr; }
	T* operator->() const { return m_Ptr; }

	Iterator& operator++() { m_Ptr++; return *this; }

	bool operator==(const Iterator& other) const { return m_Ptr == other.m_Ptr; }
private:
	T* m_Ptr;
};