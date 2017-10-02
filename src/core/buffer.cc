/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "buffer.h"
#include "mem-manager.h"

#include <iostream>
#include <assert.h>

namespace edcore
{
Buffer::Buffer(BufferNode *root)
{
    assert(root != NULL);
    this->root = root;
    MM_REGISTER(this);
}

Buffer::~Buffer()
{
    MM_UNREGISTER(this);
    delete this->root;
}

void Buffer::extractString(BufferCursor start, size_t len, uint16_t *dest)
{
    this->root->extractString(start, len, dest);
}

bool Buffer::findOffset(size_t offset, BufferCursor &result)
{
    return this->root->findOffset(offset, result);
}

bool Buffer::findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end)
{
    return this->root->findLine(lineNumber, start, end);
}

void Buffer::print(ostream &os)
{
    this->root->print(os);
}

std::ostream &operator<<(std::ostream &os, Buffer *const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}
}
