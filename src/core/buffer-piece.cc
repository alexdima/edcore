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
    this->_init(data, len, lineStarts, lineStartsCount);
}

void BufferPiece::_init(uint16_t *data, size_t len, size_t *lineStarts, size_t lineStartsCount)
{
    this->data_ = data;
    this->length_ = len;
    this->lineStarts_ = lineStarts;
    this->lineStartsCount_ = lineStartsCount;
}

BufferPiece::~BufferPiece()
{
    delete[] this->data_;
    delete[] this->lineStarts_;
}

void BufferPiece::print(std::ostream &os) const
{
    const uint16_t *data = this->data_;
    const size_t len = this->length_;
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
