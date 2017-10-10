/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_ARRAY_H_
#define EDCORE_ARRAY_H_

#include <memory>
#include <vector>
#include <cstring>
#include <stdint.h>

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

    ~MyArray()
    {
        delete[] data_;
    }

    T *data() const { return data_; }
    size_t length() const { return length_; }
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
