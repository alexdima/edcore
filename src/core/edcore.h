/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef SRC_EDCORE_H_
#define SRC_EDCORE_H_

#include <memory>
#include <vector>

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

    bool isLeaf() const;

    void setParent(BufferNode *parent);

    size_t length() const;

    size_t newLinesCount() const;

    BufferNode *firstLeaf();
    BufferNode *next();

    bool findOffset(size_t offset, BufferCursor &result);
    bool findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end);

    bool _findLineStart(size_t &lineIndex, BufferCursor &result);
    void _findLineEnd(BufferNode *node, size_t nodeStartOffset, size_t innerLineIndex, BufferCursor &result);
    void extractString(BufferCursor start, size_t len, uint16_t *dest);
};

class Buffer
{
  private:
    BufferNode *root;

  public:
    Buffer(BufferNode *root);
    ~Buffer();
    size_t length() const;
    size_t getLineCount() const;
    void print(ostream &os);

    bool findOffset(size_t offset, BufferCursor &result);
    bool findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end);
    void extractString(BufferCursor start, size_t len, uint16_t *dest);
};

class BufferBuilder
{
  private:
    vector<shared_ptr<BufferNodeString>> _rawPieces;
    bool _hasPreviousChar;
    uint16_t _previousChar;

  public:
    BufferBuilder();
    void AcceptChunk(uint16_t *chunk, size_t chunkLen);
    void Finish();
    Buffer *Build();
};
}

#endif
