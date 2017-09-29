/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <iostream>
#include <assert.h>

#include "buffer-node-string.h"
#include "mem-manager.h"

namespace edcore
{

void BufferNodeString::_init(uint16_t *data, size_t len, size_t *lineStarts, size_t lineStartsCount)
{
    this->_data = data;
    this->_len = len;
    this->_lineStarts = lineStarts;
    this->_lineStartsCount = lineStartsCount;
    MM_REGISTER(this);
}

BufferNodeString::BufferNodeString(uint16_t *data, size_t len)
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

BufferNodeString::~BufferNodeString()
{
    MM_UNREGISTER(this);
    delete[] this->_data;
    delete[] this->_lineStarts;
}

size_t BufferNodeString::getLen() const
{
    return this->_len;
}

size_t BufferNodeString::getNewLineCount() const
{
    return this->_lineStartsCount;
}

const uint16_t *BufferNodeString::getData() const // TODO
{
    return this->_data;
}

const size_t *BufferNodeString::getLineStarts() const
{
    return this->_lineStarts;
}

void BufferNodeString::print(std::ostream &os) const
{
    const uint16_t *data = this->_data;
    const size_t len = this->_len;
    for (size_t i = 0; i < len; i++)
    {
        os << data[i];
    }
}

std::ostream &operator<<(std::ostream &os, BufferNodeString *const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}
std::ostream &operator<<(std::ostream &os, shared_ptr<BufferNodeString> const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}
}
