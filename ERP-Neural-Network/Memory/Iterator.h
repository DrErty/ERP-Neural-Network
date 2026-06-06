#pragma once

#include <iterator>

#pragma once

#include <cstddef>
#include <iterator>

template<typename T>
class Iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    explicit Iterator(T* ptr)
        : m_Ptr(ptr)
    {
    }

    reference operator*() const
    {
        return *m_Ptr;
    }

    pointer operator->() const
    {
        return m_Ptr;
    }

    Iterator& operator++()
    {
        ++m_Ptr;
        return *this;
    }

    Iterator operator++(int)
    {
        Iterator temp = *this;
        ++(*this);
        return temp;
    }

    Iterator& operator--()
    {
        --m_Ptr;
        return *this;
    }

    Iterator operator--(int)
    {
        Iterator temp = *this;
        --(*this);
        return temp;
    }

    Iterator operator+(difference_type difference) const
    {
        return Iterator(m_Ptr + difference);
    }

    Iterator operator-(difference_type difference) const
    {
        return Iterator(m_Ptr - difference);
    }

    difference_type operator-(const Iterator& other) const
    {
        return m_Ptr - other.m_Ptr;
    }

    Iterator& operator+=(difference_type difference)
    {
        m_Ptr += difference;
        return *this;
    }

    Iterator& operator-=(difference_type difference)
    {
        m_Ptr -= difference;
        return *this;
    }

    reference operator[](difference_type index) const
    {
        return *(m_Ptr + index);
    }

    bool operator==(const Iterator& other) const
    {
        return m_Ptr == other.m_Ptr;
    }

    bool operator!=(const Iterator& other) const
    {
        return !(*this == other);
    }

    bool operator<(const Iterator& other) const
    {
        return m_Ptr < other.m_Ptr;
    }

    bool operator>(const Iterator& other) const
    {
        return other < *this;
    }

    bool operator<=(const Iterator& other) const
    {
        return !(other < *this);
    }

    bool operator>=(const Iterator& other) const
    {
        return !(*this < other);
    }

private:
    T* m_Ptr;
};