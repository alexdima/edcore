/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_BUFFER_BUILDER_H_
#define EDCORE_BUFFER_BUILDER_H_

#include <memory>
#include <vector>

#include "buffer-piece.h"
#include "buffer.h"

using namespace std;

namespace edcore
{

class BufferBuilder
{
  public:
    BufferBuilder();
    void AcceptChunk(uint16_t *chunk, size_t chunkLen);
    void Finish();
    Buffer *Build();

  private:
    vector<BufferPiece*> _rawPieces;
    bool _hasPreviousChar;
    uint16_t _previousChar;
};
}

#endif
