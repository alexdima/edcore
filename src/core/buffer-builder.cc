/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "buffer-builder.h"

namespace edcore
{

BufferNode *buildBufferFromPieces(vector<BufferNodeString*> &pieces, size_t start, size_t end)
{
    size_t cnt = end - start;

    if (cnt == 0)
    {
        return NULL;
    }

    if (cnt == 1)
    {
        return new BufferNode(pieces[start]);
    }

    size_t mid = (start + cnt / 2);

    BufferNode *left = buildBufferFromPieces(pieces, start, mid);
    BufferNode *right = buildBufferFromPieces(pieces, mid, end);

    BufferNode *result = new BufferNode(left, right);
    left->setParent(result);
    right->setParent(result);

    return result;
}

BufferBuilder::BufferBuilder()
{
    _hasPreviousChar = false;
    _previousChar = 0;
}

void BufferBuilder::AcceptChunk(uint16_t *chunk, size_t chunkLen)
{
    if (chunkLen == 0)
    {
        // Nothing to do
        return;
    }

    bool holdBackLastChar = false;
    uint16_t lastChar = chunk[chunkLen - 1];
    if (lastChar == 13 || (lastChar >= 0xd800 && lastChar <= 0xdbff))
    {
        // last character is \r or a high surrogate => keep it back
        holdBackLastChar = true;
    }

    size_t dataLen = (_hasPreviousChar ? 1 : 0) + chunkLen - (holdBackLastChar ? 1 : 0);
    uint16_t *data = new uint16_t[dataLen];
    if (_hasPreviousChar)
    {
        data[0] = _previousChar;
    }
    memcpy(data + (_hasPreviousChar ? 1 : 0), chunk, sizeof(uint16_t) * (chunkLen - (holdBackLastChar ? 1 : 0)));

    _rawPieces.push_back(new BufferNodeString(data, dataLen));
    _hasPreviousChar = holdBackLastChar;
    _previousChar = lastChar;
}

void BufferBuilder::Finish()
{
    if (_rawPieces.size() == 0)
    {
        // no chunks

        size_t dataLen;
        uint16_t *data;

        if (_hasPreviousChar)
        {
            _hasPreviousChar = false;
            dataLen = 1;
            data = new uint16_t[1];
            data[0] = _previousChar;
        }
        else
        {
            dataLen = 0;
            data = new uint16_t[0];
        }

        _rawPieces.push_back(new BufferNodeString(data, dataLen));

        return;
    }

    if (_hasPreviousChar)
    {
        _hasPreviousChar = false;
        // recreate last chunk

        BufferNodeString* lastPiece = _rawPieces[_rawPieces.size() - 1];
        size_t prevDataLen = lastPiece->length();
        const uint16_t *prevData = lastPiece->data();

        size_t dataLen = prevDataLen + 1;
        uint16_t *data = new uint16_t[dataLen];
        memcpy(data, prevData, sizeof(uint16_t) * prevDataLen);
        data[dataLen - 1] = _previousChar;

        delete lastPiece;

        _rawPieces[_rawPieces.size() - 1] = new BufferNodeString(data, dataLen);
    }
}

Buffer *BufferBuilder::Build()
{
    return new Buffer(_rawPieces);
}
}
