/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef SRC_BUFFER_CURSOR_H_
#define SRC_BUFFER_CURSOR_H_

#include <memory>

using namespace std;

namespace edcore
{

class BufferNode;

class BufferCursor
{
  public:
    size_t offset;
    BufferNode *node;
    size_t nodeStartOffset;

    BufferCursor() : BufferCursor(0, NULL, 0) {}
    BufferCursor(BufferCursor &src) : BufferCursor(src.offset, src.node, src.nodeStartOffset) {}
    BufferCursor(size_t offset, BufferNode *node, size_t nodeStartOffset)
    {
        this->offset = offset;
        this->node = node;
        this->nodeStartOffset = nodeStartOffset;
    }
};
}

#endif
