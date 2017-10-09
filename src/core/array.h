/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_ARRAY_H_
#define EDCORE_ARRAY_H_

#include <memory>
#include <vector>
#include <cstring>

using namespace std;

namespace edcore
{

template <class T>
class MyArray
{
  private:
    T *data_;
    size_t length_;
    size_t capacity_;

  public:
    MyArray()
    {
        data_ = NULL;
        length_ = 0;
        capacity_ = 0;
    }

    void assign(T *data, size_t length)
    {
        if (data_ != NULL)
        {
            delete[] data_;
        }
        data_ = data;
        length_ = length;
        capacity_ = length;
    }

    void assign(vector<T> &v)
    {
        T *newData = new T[v.size()];
        memcpy(newData, &v[0], sizeof(T) * v.size());
        assign(newData, v.size());
    }

    void setLength(size_t length)
    {
        length_ = length;
    }

    ~MyArray()
    {
        delete[] data_;
    }

    T *data() const { return data_; }
    size_t length() const { return length_; }
    size_t capacity() const { return capacity_; }
    size_t memUsage() const { return sizeof(MyArray) + capacity_ * sizeof(T); }

    T &operator[](size_t index) const { return data_[index]; }

    void deleteLast()
    {
        length_--;
    }

    void insertFirstElement(T element)
    {
        if (length_ + 1 <= capacity_)
        {
            // fits in place
            memmove(data_ + 0 + 1, data_ + 0, sizeof(T) * length_);
        }
        else
        {
            size_t newCapacity = length_ + 1;
            T *newData = new T[newCapacity];
            memcpy(newData, data_, sizeof(T) * 0);
            memcpy(newData + 0 + 1, data_ + 0, sizeof(T) * length_);

            delete[] data_;
            data_ = newData;
            capacity_ = newCapacity;
        }

        data_[0] = element;
        length_ = length_ + 1;
    }

    void insert(size_t offset, const T *data, size_t len)
    {
        if (length_ + len <= capacity_)
        {
            // fits in place
            memmove(data_ + offset + len, data_ + offset, sizeof(T) * length_);
        }
        else
        {
            size_t newCapacity = length_ + len;
            T *newData = new T[newCapacity];
            memcpy(newData, data_, sizeof(T) * offset);
            memcpy(newData + offset + len, data_ + offset, sizeof(T) * length_);

            delete[] data_;
            data_ = newData;
            capacity_ = newCapacity;
        }

        memcpy(data_ + offset, data, sizeof(T) * len);
        length_ = length_ + len;
    }

    void deleteRange(size_t deleteFrom, size_t deleteCount)
    {
        size_t deleteTo = deleteFrom + deleteCount;
        if (deleteTo < length_)
        {
            // there are still elements after the delete
            memmove(data_ + deleteFrom, data_ + deleteTo, sizeof(T) * (length_ - deleteTo));
        }
        length_ -= (deleteTo - deleteFrom);
    }

    void append(const T *source, size_t sourceLength)
    {
        if (length_ + sourceLength > capacity_)
        {
            size_t newCapacity = length_ + sourceLength;
            T *newData = new T[newCapacity];
            memcpy(newData, data_, sizeof(T) * length_);

            delete[] data_;
            data_ = newData;
            capacity_ = newCapacity;
        }

        memcpy(data_ + length_, source, sizeof(T) * sourceLength);
        length_ = length_ + sourceLength;
    }
};
}

#endif
