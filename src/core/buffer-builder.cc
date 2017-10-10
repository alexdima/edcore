/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "buffer-builder.h"

#include <cstring>

namespace edcore
{

BufferBuilder::BufferBuilder()
{
    hasPreviousChar_ = false;
    averageChunkSize_ = 0;
    previousChar_ = 0;
}

void BufferBuilder::acceptChunk(const BufferString *str)
{
    const size_t strLength = str->length();
    if (strLength == 0)
    {
        // Nothing to do
        return;
    }

    // Current pieces
    const size_t rawPiecesCount = rawPieces_.size();
    averageChunkSize_ = (averageChunkSize_ * rawPiecesCount + strLength) / (rawPiecesCount + 1);

    bool holdBackLastChar = false;

    uint16_t lastChar;
    str->write(&lastChar, strLength - 1, 1);

    if (lastChar == 13 || (lastChar >= 0xd800 && lastChar <= 0xdbff))
    {
        // last character is \r or a high surrogate => keep it back
        holdBackLastChar = true;
    }

    size_t dataLen = (hasPreviousChar_ ? 1 : 0) + strLength - (holdBackLastChar ? 1 : 0);
    uint16_t *data = new uint16_t[dataLen];
    if (hasPreviousChar_)
    {
        data[0] = previousChar_;
    }
    str->write(data + (hasPreviousChar_ ? 1 : 0), 0, strLength - (holdBackLastChar ? 1 : 0));

    rawPieces_.push_back(new TwoBytesBufferPiece(data, dataLen));
    hasPreviousChar_ = holdBackLastChar;
    previousChar_ = lastChar;
}

void BufferBuilder::finish()
{
    if (rawPieces_.size() == 0)
    {
        // no chunks

        size_t dataLen;
        uint16_t *data;

        if (hasPreviousChar_)
        {
            hasPreviousChar_ = false;
            dataLen = 1;
            data = new uint16_t[1];
            data[0] = previousChar_;
        }
        else
        {
            dataLen = 0;
            data = new uint16_t[0];
        }

        rawPieces_.push_back(new TwoBytesBufferPiece(data, dataLen));

        return;
    }

    if (hasPreviousChar_)
    {
        hasPreviousChar_ = false;
        // recreate last chunk

        BufferPiece *lastPiece = rawPieces_[rawPieces_.size() - 1];
        size_t prevDataLen = lastPiece->length();

        size_t dataLen = prevDataLen + 1;
        uint16_t *data = new uint16_t[dataLen];
        lastPiece->write(data, 0, prevDataLen);
        data[dataLen - 1] = previousChar_;

        delete lastPiece;

        rawPieces_[rawPieces_.size() - 1] = new TwoBytesBufferPiece(data, dataLen);
    }
}

Buffer *BufferBuilder::build()
{
    size_t averageChunkSize = (size_t)min(65536.0, max(128.0, averageChunkSize_));
    size_t delta = averageChunkSize / 3;
    size_t min_ = averageChunkSize - delta;
    size_t max_ = 2 * min_;

    // printf("%lf ==> %lu, %lu\n", averageChunkSize_, min_, max_);

    return new Buffer(rawPieces_, min_, max_);
}
}
