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

    size_t *lineStarts = new size_t[lineStartsCount];

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

void BufferPiece::_init(uint16_t *data, size_t len, size_t *lineStarts, size_t lineStartsCount)
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

void BufferPiece::deleteOneOffsetLen(size_t offset, size_t len)
{
    cout << "Line starts: ";
    size_t deleteFrom = lineStartsCount_;
    size_t deleteTo = 0;
    for (size_t i = 0; i < lineStartsCount_; i++)
    {
        size_t lineStart = lineStarts_[i];
        if (lineStart <= offset)
        {
            continue;
        }
        if (lineStart > offset + len)
        {
            lineStarts_[i] -= len;
            continue;
        }

        // Cover the case of deleting only the \n in a \r\n pair
        if (offset == lineStart - 1 && lineStart > 1 && data_[lineStart - 2] == '\r' && data_[lineStart - 1] == '\n')
        {
            // The line start remains
            lineStarts_[i] -= 1;
            continue;
        }

        // This line start must be deleted
        deleteFrom = min(deleteFrom, i);
        deleteTo = max(deleteTo, i + 1);
        // bool deleteLineStart = true;

        // // if ()
        // assert(false);

        // uint16_t char1 = data_[lineStart - 1];
        // uint16_t char0 = lineStart > 1 ? data_[lineStart - 2] : 0;

        // // if (lineStart)
        // if (lineStart > offset)
        // {
        //     lineStarts_[i] -= len;
        // }
        // else
        // {
        //     // TODO
        //     assert(false);
        // }
        // // cout << " - lineStart["<<i<<"]: " << lineStarts_[i] << endl;
        // // break;
    }


    if (deleteFrom < deleteTo)
    {
        memcpy(lineStarts_ + deleteFrom, lineStarts_ + deleteTo, sizeof(size_t) * (lineStartsCount_ - deleteTo));
        lineStartsCount_ -= (deleteTo - deleteFrom);
        // printf("\nTODO!!!!!\n");
    }

    uint16_t length = length_ - len;
    memcpy(data_ + offset, data_ + offset + len, sizeof(uint16_t) * (length - offset));
    length_ = length;

    printf("INSIDE LEAF, deleteOneOffsetLen %lu %lu\n", offset, len);
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
