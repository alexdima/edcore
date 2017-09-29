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

void printIndent(ostream &os, int indent)
{
    for (int i = 0; i < indent; i++)
    {
        os << " ";
    }
}


void BufferNode::_init(
    shared_ptr<BufferNodeString> str,
    BufferNode *leftChild,
    BufferNode *rightChild,
    size_t len,
    size_t newLineCount)
{
    this->_str = str;
    this->_leftChild = leftChild;
    this->_rightChild = rightChild;
    this->_parent = NULL;
    this->_len = len;
    this->_newLineCount = newLineCount;
    MM_REGISTER(this);
}

BufferNode::BufferNode(shared_ptr<BufferNodeString> str)
{
    assert(str != NULL);
    this->_init(str, NULL, NULL, str->length(), str->newLinesCount());
}

BufferNode::BufferNode(BufferNode *leftChild, BufferNode *rightChild)
{
    assert(leftChild != NULL || rightChild != NULL);

    const size_t len = (leftChild != NULL ? leftChild->_len : 0) + (rightChild != NULL ? rightChild->_len : 0);
    const size_t newLineCount = (leftChild != NULL ? leftChild->_newLineCount : 0) + (rightChild != NULL ? rightChild->_newLineCount : 0);

    this->_init(NULL, leftChild, rightChild, len, newLineCount);
}

BufferNode::~BufferNode()
{
    MM_UNREGISTER(this);
    // if (this->_str != NULL)
    // {
    //     delete this->_str;
    //     this->_str = NULL;
    // }
    if (this->_leftChild != NULL)
    {
        delete this->_leftChild;
        this->_leftChild = NULL;
    }
    if (this->_rightChild != NULL)
    {
        delete this->_rightChild;
        this->_rightChild = NULL;
    }
    this->_parent = NULL;
}

void BufferNode::print(ostream &os)
{
    this->log(os, 0);
}

void BufferNode::log(ostream &os, int indent)
{
    if (this->isLeaf())
    {
        printIndent(os, indent);
        os << "[LEAF] (len:" << this->_len << ", newLineCount:" << this->_newLineCount << ")" << endl;
        return;
    }

    printIndent(os, indent);
    os << "[NODE] (len:" << this->_len << ", newLineCount:" << this->_newLineCount << ")" << endl;

    indent += 4;
    if (this->_leftChild)
    {
        this->_leftChild->log(os, indent);
    }
    if (this->_rightChild)
    {
        this->_rightChild->log(os, indent);
    }
}

bool BufferNode::isLeaf() const
{
    return (this->_str != NULL);
}

void BufferNode::setParent(BufferNode *parent)
{
    this->_parent = parent;
}

size_t BufferNode::length() const
{
    return this->_len;
}

size_t BufferNode::newLinesCount() const
{
    return this->_newLineCount;
}

BufferNode *BufferNode::findPieceAtOffset(size_t &offset)
{
    if (offset >= this->_len)
    {
        return NULL;
    }

    BufferNode *it = this;
    while (!it->isLeaf())
    {
        BufferNode *left = it->_leftChild;
        BufferNode *right = it->_rightChild;

        if (left == NULL)
        {
            it = right;
            continue;
        }

        if (right == NULL)
        {
            it = left;
            continue;
        }

        const size_t leftLen = left->_len;
        if (offset < leftLen)
        {
            // go left
            it = left;
            continue;
        }

        // go right
        offset -= leftLen;
        it = right;
    }

    return it;
}

BufferNode *BufferNode::firstLeaf()
{
    // TODO: this will not work for an unbalanced tree
    BufferNode *res = this;
    while (res->_leftChild != NULL)
    {
        res = res->_leftChild;
    }
    return res;
}

BufferNode *BufferNode::next()
{
    assert(this->isLeaf());
    if (this->_parent->_leftChild == this)
    {
        BufferNode *sibling = this->_parent->_rightChild;
        return sibling->firstLeaf();
    }

    BufferNode *it = this;
    while (it->_parent != NULL && it->_parent->_rightChild == it)
    {
        it = it->_parent;
    }
    if (it->_parent == NULL)
    {
        // EOF
        return NULL;
    }
    return it->_parent->_rightChild->firstLeaf();
}

void BufferNode::extractString(BufferCursor start, size_t len, uint16_t *dest)
{
    size_t innerNodeOffset = start.offset - start.nodeStartOffset;
    BufferNode *node = start.node;

    if (innerNodeOffset + len <= node->_len)
    {
        // This is a simple substring
        const uint16_t *data = node->_str->data();
        memcpy(dest, data + innerNodeOffset, sizeof(uint16_t) * len);
        return;
    }

    // uint16_t *result = new uint16_t[len];
    size_t resultOffset = 0;
    size_t remainingLen = len;
    do
    {
        const uint16_t *src = node->_str->data();
        const size_t cnt = min(remainingLen, node->_str->length() - innerNodeOffset);
        memcpy(dest + resultOffset, src + innerNodeOffset, sizeof(uint16_t) * cnt);
        remainingLen -= cnt;
        resultOffset += cnt;
        innerNodeOffset = 0;

        if (remainingLen == 0)
        {
            break;
        }

        node = node->next();
        assert(node->isLeaf());
    } while (true);
}

bool BufferNode::findOffset(size_t offset, BufferCursor &result)
{
    if (offset > this->_len)
    {
        return false;
    }

    BufferNode *it = this;
    size_t searchOffset = offset;
    size_t nodeStartOffset = 0;
    while (!it->isLeaf())
    {
        BufferNode *left = it->_leftChild;
        BufferNode *right = it->_rightChild;

        if (left == NULL)
        {
            it = right;
            continue;
        }

        if (right == NULL)
        {
            it = left;
            continue;
        }

        if (searchOffset < left->_len)
        {
            // go left
            it = left;
            continue;
        }

        // go right
        searchOffset -= left->_len;
        nodeStartOffset += left->_len;
        it = right;
    }

    result.offset = offset;
    result.node = it;
    result.nodeStartOffset = nodeStartOffset;

    return true;
}

bool BufferNode::_findLineStart(size_t &lineIndex, BufferCursor &result)
{
    if (lineIndex > this->_newLineCount)
    {
        return false;
    }

    BufferNode *it = this;
    size_t nodeStartOffset = 0;
    while (!it->isLeaf())
    {
        BufferNode *left = it->_leftChild;
        BufferNode *right = it->_rightChild;

        if (left == NULL)
        {
            // go right
            it = right;
        }

        if (right == NULL)
        {
            // go left
            it = left;
        }

        if (lineIndex <= left->_newLineCount)
        {
            // go left
            it = left;
            continue;
        }

        // go right
        lineIndex -= left->_newLineCount;
        nodeStartOffset += left->_len;
        it = right;
    }

    const size_t *lineStarts = it->_str->lineStarts();
    const size_t innerLineStartOffset = (lineIndex == 0 ? 0 : lineStarts[lineIndex - 1]);

    result.offset = nodeStartOffset + innerLineStartOffset;
    result.node = it;
    result.nodeStartOffset = nodeStartOffset;

    return true;
}

void BufferNode::_findLineEnd(BufferNode *node, size_t nodeStartOffset, size_t innerLineIndex, BufferCursor &result)
{
    const size_t nodeLineCount = node->_newLineCount;
    assert(node->isLeaf());
    assert(nodeLineCount >= innerLineIndex);

    if (innerLineIndex < nodeLineCount)
    {
        // lucky, the line ends in this same node
        const size_t *lineStarts = node->_str->lineStarts();
        size_t lineEndOffset = lineStarts[innerLineIndex];

        result.offset = nodeStartOffset + lineEndOffset;
        result.node = node;
        result.nodeStartOffset = nodeStartOffset;
        return;
    }

    // find the first newline or EOF
    size_t offset = nodeStartOffset + node->_len;
    do
    {
        // TODO: could probably optimize to not visit every leaf!!!
        BufferNode *next = node->next();

        if (next == NULL)
        {
            // EOF
            break;
        }

        nodeStartOffset += node->_len;
        node = next;

        if (node->_newLineCount > 0)
        {
            const size_t *lineStarts = node->_str->lineStarts();
            offset = nodeStartOffset + lineStarts[0];
            break;
        }
        else
        {
            offset = nodeStartOffset + node->_len;
        }
    } while (true);

    result.offset = offset;
    result.node = node;
    result.nodeStartOffset = nodeStartOffset;
}

bool BufferNode::findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end)
{
    size_t innerLineIndex = lineNumber - 1;
    if (!_findLineStart(innerLineIndex, start))
    {
        return false;
    }

    _findLineEnd(start.node, start.nodeStartOffset, innerLineIndex, end);
    return true;
}

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
