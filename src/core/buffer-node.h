/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef SRC_BUFFER_NODE_H_
#define SRC_BUFFER_NODE_H_

#include <memory>

#include "buffer-node-string.h"
#include "buffer-cursor.h"

using namespace std;

namespace edcore
{

class BufferNode
{
  private:
    shared_ptr<BufferNodeString> _str;

    BufferNode *_leftChild;
    BufferNode *_rightChild;

    BufferNode *_parent;
    size_t _len;
    size_t _newLineCount;

    void _init(shared_ptr<BufferNodeString> str, BufferNode *leftChild, BufferNode *rightChild, size_t len, size_t newLineCount);

  public:
    BufferNode(shared_ptr<BufferNodeString> str);

    BufferNode(BufferNode *leftChild, BufferNode *rightChild);

    ~BufferNode();

    void print(ostream &os);

    void log(ostream &os, int indent);

    bool isLeaf() const { return (this->_str != NULL); }

    void setParent(BufferNode *parent) { this->_parent = parent; }

    size_t length() const { return this->_len; }

    size_t newLinesCount() const { return this->_newLineCount; }

    BufferNode *firstLeaf();
    BufferNode *next();

    bool findOffset(size_t offset, BufferCursor &result);
    bool findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end);

    bool _findLineStart(size_t &lineIndex, BufferCursor &result);
    void _findLineEnd(BufferNode *node, size_t nodeStartOffset, size_t innerLineIndex, BufferCursor &result);
    void extractString(BufferCursor start, size_t len, uint16_t *dest);
};
}

#endif
