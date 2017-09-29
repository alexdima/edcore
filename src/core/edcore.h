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
#include "buffer-node.h"

using namespace std;

namespace edcore
{

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
