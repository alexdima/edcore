/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_BUFFER_NODE_H_
#define EDCORE_BUFFER_NODE_H_

#include <memory>

#include "buffer-node-string.h"
#include "buffer-cursor.h"

using namespace std;

namespace edcore
{

class BufferNode
{
  public:
    BufferNode(shared_ptr<BufferNodeString> str);
    BufferNode(BufferNode *leftChild, BufferNode *rightChild);
    ~BufferNode();

    void print(ostream &os);

    bool isLeaf() const { return (this->str_ != NULL); }
    void setParent(BufferNode *parent) { this->parent_ = parent; }
    size_t length() const { return this->length_; }
    size_t newLinesCount() const { return this->newLineCount_; }

    BufferNode *firstLeaf();
    BufferNode *next();

    bool findOffset(size_t offset, BufferCursor &result);
    bool findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end);
    void extractString(BufferCursor start, size_t len, uint16_t *dest);

  private:
    shared_ptr<BufferNodeString> str_;

    BufferNode *leftChild_;
    BufferNode *rightChild_;

    BufferNode *parent_;
    size_t length_;
    size_t newLineCount_;

    void _init(shared_ptr<BufferNodeString> str, BufferNode *leftChild, BufferNode *rightChild, size_t len, size_t newLineCount);
    void _log(ostream &os, int indent);
    bool _findLineStart(size_t &lineIndex, BufferCursor &result);
    void _findLineEnd(BufferNode *node, size_t nodeStartOffset, size_t innerLineIndex, BufferCursor &result);
};
}

#endif
