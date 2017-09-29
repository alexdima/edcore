/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "edcore.h"

#include <iostream>
// #include <memory>

// #include <fstream>
#include <vector>
// #include <cstring>
// #include <stdint.h>
#include <assert.h>
// #include <stdlib.h>

#include "mem-manager.h"

using namespace std;

#define PIECE_SIZE 4 * 1024 // 4KB

namespace edcore
{

class BufferNodeString;
class BufferNode;
class Buffer;






Buffer::Buffer(BufferNode *root)
{
    assert(root != NULL);
    this->root = root;
    MM_REGISTER(this);
}

Buffer::~Buffer()
{
    MM_UNREGISTER(this);
    delete this->root;
}

size_t Buffer::length() const
{
    return this->root->length();
}

size_t Buffer::getLineCount() const
{
    return this->root->newLinesCount() + 1;
}

void Buffer::extractString(BufferCursor start, size_t len, uint16_t *dest)
{
    this->root->extractString(start, len, dest);
}

bool Buffer::findOffset(size_t offset, BufferCursor &result)
{
    return this->root->findOffset(offset, result);
}

bool Buffer::findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end)
{
    return this->root->findLine(lineNumber, start, end);
}

void Buffer::print(ostream &os)
{
    this->root->print(os);
}

std::ostream &operator<<(std::ostream &os, Buffer *const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}

BufferNode *buildBufferFromPieces(vector<shared_ptr<BufferNodeString>> &pieces, size_t start, size_t end)
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
    cout << "~~~~~~~~~~~~~" << endl
         << "AcceptChunk called " << chunkLen << endl;

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
        cout << "~~~~~~~~~~~~~" << endl
             << "Holding back last character " << endl;
        holdBackLastChar = true;
    }

    size_t dataLen = (_hasPreviousChar ? 1 : 0) + chunkLen - (holdBackLastChar ? 1 : 0);
    uint16_t *data = new uint16_t[dataLen];
    if (_hasPreviousChar)
    {
        data[0] = _previousChar;
    }
    memcpy(data + (_hasPreviousChar ? 1 : 0), chunk, sizeof(uint16_t) * (chunkLen - (holdBackLastChar ? 1 : 0)));

    _rawPieces.push_back(shared_ptr<BufferNodeString>(new BufferNodeString(data, dataLen)));
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

        _rawPieces.push_back(shared_ptr<BufferNodeString>(new BufferNodeString(data, dataLen)));

        return;
    }

    if (_hasPreviousChar)
    {
        _hasPreviousChar = false;
        // recreate last chunk

        shared_ptr<BufferNodeString> lastPiece = _rawPieces[_rawPieces.size() - 1];
        size_t prevDataLen = lastPiece->length();
        const uint16_t *prevData = lastPiece->data();

        size_t dataLen = prevDataLen + 1;
        uint16_t *data = new uint16_t[dataLen];
        memcpy(data, prevData, sizeof(uint16_t) * prevDataLen);
        data[dataLen - 1] = _previousChar;

        _rawPieces[_rawPieces.size() - 1] = shared_ptr<BufferNodeString>(new BufferNodeString(data, dataLen));
    }
}

Buffer *BufferBuilder::Build()
{
    size_t pieceCount = _rawPieces.size();
    BufferNode *root = buildBufferFromPieces(_rawPieces, 0, pieceCount);
    return new Buffer(root);
}

// Buffer *buildBufferFromFile(const char *filename)
// {
//     ifstream ifs(filename, ifstream::binary);
//     if (!ifs)
//     {
//         return NULL;
//     }
//     vector<shared_ptr<BufferNodeString>> rawPieces;
//     ifs.seekg(0, std::ios::beg);
//     while (!ifs.eof())
//     {
//         char *piece = new char[PIECE_SIZE];

//         ifs.read(piece, PIECE_SIZE);

//         if (ifs)
//         {
//             rawPieces.push_back(shared_ptr<BufferNodeString>(new BufferNodeString(piece, PIECE_SIZE)));
//         }
//         else
//         {
//             rawPieces.push_back(shared_ptr<BufferNodeString>(new BufferNodeString(piece, ifs.gcount())));
//         }
//     }
//     ifs.close();

//     size_t pieceCount = rawPieces.size();
//     BufferNode *root = buildBufferFromPieces(rawPieces, 0, pieceCount);
//     return new Buffer(root);
// }

timespec diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

double took(timespec start, timespec end)
{
    timespec d = diff(start, end);
    return d.tv_sec * 1000 + d.tv_nsec / (double)1000000;
}

timespec _tmp_timespec;

#define TIME_START(name) \
    timespec name;       \
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &name)

#define TIME_END(os, name, explanation)                      \
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &_tmp_timespec); \
    os << explanation << " took " << took(name, _tmp_timespec) << " ms." << endl
}
