/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <iostream>
#include <assert.h>

#include "buffer-piece.h"

namespace edcore
{

BufferPiece::BufferPiece(uint16_t *data, size_t len)
{
    assert(data != NULL && len != 0);

    // Do a first pass to count the number of line starts
    size_t lineStartsCount = 0;
    for (size_t i = 0; i < len; i++)
    {
        uint16_t chr = data[i];

        if (chr == '\r')
        {
            if (i + 1 < len && data[i + 1] == '\n')
            {
                // \r\n... case
                lineStartsCount++;
                i++; // skip \n
            }
            else
            {
                // \r... case
                lineStartsCount++;
            }
        }
        else if (chr == '\n')
        {
            lineStartsCount++;
        }
    }

    LINE_START_T *lineStarts = new LINE_START_T[lineStartsCount];

    size_t dest = 0;
    for (size_t i = 0; i < len; i++)
    {
        uint16_t chr = data[i];

        if (chr == '\r')
        {
            if (i + 1 < len && data[i + 1] == '\n')
            {
                // \r\n... case
                lineStarts[dest++] = i + 2;
                i++; // skip \n
            }
            else
            {
                // \r... case
                lineStarts[dest++] = i + 1;
            }
        }
        else if (chr == '\n')
        {
            lineStarts[dest++] = i + 1;
        }
    }
    _init(data, len, lineStarts, lineStartsCount);
}

void BufferPiece::_init(uint16_t *data, size_t len, LINE_START_T *lineStarts, size_t lineStartsCount)
{
    data_ = data;
    length_ = len;
    dataCapacity_ = len;

    lineStarts_ = lineStarts;
    lineStartsCount_ = lineStartsCount;
    lineStartsCapacity_ = lineStartsCount;
}

BufferPiece::~BufferPiece()
{
    delete[] data_;
    delete[] lineStarts_;
}

uint16_t BufferPiece::deleteLastChar()
{
    assert(length_ > 0);

    uint16_t ret = data_[length_ - 1];

    if (lineStartsCount_ > 0 && lineStarts_[lineStartsCount_ - 1] == length_)
    {
        lineStartsCount_--;
    }
    length_--;

    return ret;
}

void BufferPiece::insertFirstChar(uint16_t character)
{
    bool insertLineStart = false;
    if (character == '\r' && (lineStartsCount_ == 0 || lineStarts_[0] != 1 || data_[0] != '\n'))
    {
        insertLineStart = true;
    }

    for (size_t i = 0; i < lineStartsCount_; i++)
    {
        lineStarts_[i] += 1;
    }

    if (insertLineStart)
    {
        if (lineStartsCapacity_ > lineStartsCount_)
        {
            memmove(lineStarts_ + 1, lineStarts_, sizeof(LINE_START_T) * lineStartsCount_);
            lineStarts_[0] = 1;
            lineStartsCount_ = lineStartsCount_ + 1;
        }
        else
        {
            uint16_t lineStartsCount = lineStartsCount_ + 1;
            LINE_START_T *newLineStarts = new LINE_START_T[lineStartsCount];
            memcpy(newLineStarts + 1, lineStarts_, sizeof(LINE_START_T) * lineStartsCount_);
            newLineStarts[0] = 1;
            delete[] lineStarts_;

            lineStarts_ = newLineStarts;
            lineStartsCapacity_ = lineStartsCount;
            lineStartsCount_ = lineStartsCount;
        }
    }

    if (dataCapacity_ > length_)
    {
        memmove(data_ + 1, data_, sizeof(uint16_t) * length_);
        data_[0] = character;
        length_ = length_ + 1;
    }
    else
    {
        uint16_t length = length_ + 1;
        uint16_t *newData = new uint16_t[length];
        memcpy(newData + 1, data_, sizeof(uint16_t) * length_);
        newData[0] = character;
        delete[] data_;

        data_ = newData;
        dataCapacity_ = length;
        length_ = length;
    }
}

void BufferPiece::deleteOneOffsetLen(size_t offset, size_t len)
{
    assert(offset + len <= length_);

    size_t deleteLineStartsFrom = lineStartsCount_;
    size_t deleteLineStartsTo = 0;
    LINE_START_T *deletingCase1 = NULL;
    for (size_t i = 0; i < lineStartsCount_; i++)
    {
        LINE_START_T lineStart = lineStarts_[i];
        if (lineStart < offset)
        {
            // Entirely before deletion
            continue;
        }
        if (lineStart > offset + len + 1)
        {
            // Entirely after deletion
            lineStarts_[i] -= len;
            continue;
        }

        // Boundary: Cover the case of deleting: \r[\n...]
        if (offset == lineStart - 1 && lineStart > 1 && data_[lineStart - 2] == '\r' && data_[lineStart - 1] == '\n')
        {
            // The line start remains
            lineStarts_[i] -= 1;
            deletingCase1 = &lineStarts_[i];
            continue;
        }

        // Boundary: Cover the case of deleting: \r[...]
        if (offset == lineStart)
        {
            if (data_[lineStart - 1] == '\r')
            {
                deletingCase1 = &lineStarts_[i];
            }
            continue;
        }

        // Boundary: Cover the case of deleting: \r[...\r]\n
        if (offset + len == lineStart - 1)
        {
            if (deletingCase1 != NULL && lineStart > 1 && data_[lineStart - 2] == '\r' && data_[lineStart - 1] == '\n')
            {
                (*deletingCase1) = (*deletingCase1) + 1;
            }
            else
            {
                // The line start remains
                lineStarts_[i] -= len;
                continue;
            }
        }

        // This line start must be deleted
        deleteLineStartsFrom = min(deleteLineStartsFrom, i);
        deleteLineStartsTo = max(deleteLineStartsTo, i + 1);
    }

    if (deleteLineStartsFrom < deleteLineStartsTo)
    {
        memmove(lineStarts_ + deleteLineStartsFrom, lineStarts_ + deleteLineStartsTo, sizeof(LINE_START_T) * (lineStartsCount_ - deleteLineStartsTo));
        lineStartsCount_ -= (deleteLineStartsTo - deleteLineStartsFrom);
    }

    uint16_t length = length_ - len;
    memmove(data_ + offset, data_ + offset + len, sizeof(uint16_t) * (length - offset));
    length_ = length;
}

void BufferPiece::insertOneOffsetLen(size_t offset, const uint16_t *data, size_t len)
{
    printf("TODO: insertOneOffsetLen %lu (data of %lu)\n", offset, len);
}

void BufferPiece::print(std::ostream &os) const
{
    const uint16_t *data = data_;
    const size_t len = length_;
    for (size_t i = 0; i < len; i++)
    {
        os << data[i];
    }
}

std::ostream &operator<<(std::ostream &os, BufferPiece *const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}
}
