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

uint16_t getLastCharacter(const BufferString *str)
{
    uint16_t lastChar;
    str->write(&lastChar, str->length() - 1, 1);
    return lastChar;
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

    uint16_t lastChar = getLastCharacter(str);
    if (lastChar == 13 || (lastChar >= 0xd800 && lastChar <= 0xdbff))
    {
        // last character is \r or a high surrogate => keep it back
        BufferString *tmp = BufferString::substr(str, 0, str->length() - 1);
        acceptChunk1(tmp, false);
        delete tmp;
        hasPreviousChar_ = true;
        previousChar_ = lastChar;
    }
    else
    {
        acceptChunk1(str, false);
        hasPreviousChar_ = false;
        previousChar_ = lastChar;
    }
}

void BufferBuilder::acceptChunk1(const BufferString *str, bool allowEmptyStrings)
{
    const size_t strLength = str->length();
    if (!allowEmptyStrings && strLength == 0)
    {
        // Nothing to do
        return;
    }

    if (hasPreviousChar_)
    {
        BufferString *tmp1 = BufferString::createFromSingle(previousChar_);
        BufferString *tmp2 = BufferString::concat(tmp1, str);
        acceptChunk2(tmp2);
        delete tmp2;
        delete tmp1;
    }
    else
    {
        acceptChunk2(str);
    }
}

void BufferBuilder::acceptChunk2(const BufferString *str)
{
    rawPieces_.push_back(BufferPiece::createFromString(str));
}

void BufferBuilder::finish()
{
    if (rawPieces_.size() == 0)
    {
        // no chunks => forcefully go through accept chunk
        acceptChunk1(BufferString::empty(), true);
        return;
    }

    if (hasPreviousChar_)
    {
        hasPreviousChar_ = false;

        // recreate last chunk
        BufferPiece *lastPiece = rawPieces_[rawPieces_.size() - 1];

        BufferString *tmp1 = BufferString::createFromSingle(previousChar_);
        BufferPiece *tmp2 = BufferPiece::createFromString(tmp1);
        delete tmp1;

        BufferPiece *newLastPiece = BufferPiece::join2(lastPiece, tmp2);
        delete lastPiece;
        delete tmp2;

        rawPieces_[rawPieces_.size() - 1] = newLastPiece;
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
