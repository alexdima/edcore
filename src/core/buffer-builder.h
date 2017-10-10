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
    void acceptChunk(const BufferString *str);
    void finish();
    Buffer *build();

  private:
    vector<BufferPiece *> rawPieces_;
    bool hasPreviousChar_;
    uint16_t previousChar_;
    double averageChunkSize_;

    void acceptChunk1(const BufferString *str, bool allowEmptyStrings);
    void acceptChunk2(const BufferString *str);
};
}

#endif
