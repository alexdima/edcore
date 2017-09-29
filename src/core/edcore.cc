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

class SimpleString;
class BufferString;
class BufferStringSubstring;
class BufferNode;
class Buffer;

std::ostream &operator<<(std::ostream &os, String *const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}
std::ostream &operator<<(std::ostream &os, shared_ptr<String> const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}

class SimpleString : public String
{
  private:
    uint16_t *_data;
    size_t _len;

  public:
    SimpleString(uint16_t *data, size_t len)
    {
        this->_data = data;
        this->_len = len;
        MM_REGISTER(this);
    }

    ~SimpleString()
    {
        MM_UNREGISTER(this);
        if (this->_data != NULL)
        {
            delete[] this->_data;
            this->_data = NULL;
        }
    }

    void print(std::ostream &os) const
    {
        const uint16_t *data = this->_data;
        const size_t len = this->_len;
        for (size_t i = 0; i < len; i++)
        {
            os << data[i];
        }
    }

    size_t getLen() const
    {
        return this->_len;
    }

    void writeTo(uint16_t *dest) const
    {
        memcpy(dest, this->_data, sizeof(uint16_t) * this->_len);
    }
};

void printIndent(ostream &os, int indent)
{
    for (int i = 0; i < indent; i++)
    {
        os << " ";
    }
}

void BufferString::_init(uint16_t *data, size_t len, size_t *lineStarts, size_t lineStartsCount)
{
    this->_data = data;
    this->_len = len;
    this->_lineStarts = lineStarts;
    this->_lineStartsCount = lineStartsCount;
    MM_REGISTER(this);
}

BufferString::BufferString(uint16_t *data, size_t len)
{
    assert(data != NULL && len != 0);

    // Do a first pass to count the number of line starts
    size_t lineStartsCount = 0;
    for (size_t i = 0; i < len; i++)
    {
        uint16_t chr = data[i];

        if (chr == '\r')
        {
            if (i + 1 < len && data[i + 1] == '\n')
            {
                // \r\n... case
                lineStartsCount++;
                i++; // skip \n
            }
            else
            {
                // \r... case
                lineStartsCount++;
            }
        }
        else if (chr == '\n')
        {
            lineStartsCount++;
        }
    }

    size_t *lineStarts = new size_t[lineStartsCount];

    size_t dest = 0;
    for (size_t i = 0; i < len; i++)
    {
        uint16_t chr = data[i];

        if (chr == '\r')
        {
            if (i + 1 < len && data[i + 1] == '\n')
            {
                // \r\n... case
                lineStarts[dest++] = i + 2;
                i++; // skip \n
            }
            else
            {
                // \r... case
                lineStarts[dest++] = i + 1;
            }
        }
        else if (chr == '\n')
        {
            lineStarts[dest++] = i + 1;
        }
    }
    this->_init(data, len, lineStarts, lineStartsCount);
}

BufferString::~BufferString()
{
    MM_UNREGISTER(this);
    if (this->_data != NULL)
    {
        delete[] this->_data;
        this->_data = NULL;
    }
    if (this->_lineStarts != NULL)
    {
        delete[] this->_lineStarts;
        this->_lineStarts = NULL;
    }
}

size_t BufferString::getLen() const
{
    return this->_len;
}

size_t BufferString::getNewLineCount() const
{
    return this->_lineStartsCount;
}

bool BufferString::getEndsWithCR() const
{
    return (this->_len > 0 && this->_data[this->_len - 1] == '\r');
}

bool BufferString::getStartsWithLF() const
{
    return (this->_len > 0 && this->_data[0] == '\n');
}

const uint16_t *BufferString::getData() const // TODO
{
    return this->_data;
}

const size_t *BufferString::getLineStarts() const
{
    return this->_lineStarts;
}

void BufferString::print(std::ostream &os) const
{
    const uint16_t *data = this->_data;
    const size_t len = this->_len;
    for (size_t i = 0; i < len; i++)
    {
        os << data[i];
    }
}

void BufferString::writeTo(uint16_t *dest) const
{
    memcpy(dest, this->_data, sizeof(uint16_t) * this->_len);
}

class BufferStringSubstring : public String
{
  private:
    shared_ptr<BufferString> _str;
    size_t _offset;
    size_t _len;

  public:
    BufferStringSubstring(shared_ptr<BufferString> str, size_t offset, size_t len)
    {
        this->_str = str;
        this->_offset = offset;
        this->_len = len;
        MM_REGISTER(this);
    }

    ~BufferStringSubstring()
    {
        MM_UNREGISTER(this);
    }

    void print(std::ostream &os) const
    {
        const uint16_t *data = this->_str->getData();
        const size_t startOffset = this->_offset;
        const size_t endOffset = this->_offset + this->_len;
        for (size_t i = startOffset; i < endOffset; i++)
        {
            os << data[i];
        }
    }

    size_t getLen() const
    {
        return this->_len;
    }

    void writeTo(uint16_t *dest) const
    {
        const uint16_t *data = this->_str->getData();
        const size_t startOffset = this->_offset;
        memcpy(dest, data + startOffset, sizeof(uint16_t) * this->_len);
    }
};
void BufferNode::_init(
    shared_ptr<BufferString> str,
    BufferNode *leftChild,
    BufferNode *rightChild,
    size_t len,
    size_t newLineCount,
    bool startsWithLF,
    bool endsWithCR)
{
    this->_str = str;
    this->_leftChild = leftChild;
    this->_rightChild = rightChild;
    this->_parent = NULL;
    this->_len = len;
    this->_newLineCount = newLineCount;
    this->_startsWithLF = startsWithLF;
    this->_endsWithCR = endsWithCR;
    MM_REGISTER(this);
}

BufferNode::BufferNode(shared_ptr<BufferString> str)
{
    assert(str != NULL);
    this->_init(str, NULL, NULL, str->getLen(), str->getNewLineCount(), str->getStartsWithLF(), str->getEndsWithCR());
}

BufferNode::BufferNode(BufferNode *leftChild, BufferNode *rightChild)
{
    assert(leftChild != NULL || rightChild != NULL);

    const size_t len = (leftChild != NULL ? leftChild->_len : 0) + (rightChild != NULL ? rightChild->_len : 0);
    const size_t newLineCount = (leftChild != NULL ? leftChild->_newLineCount : 0) + (rightChild != NULL ? rightChild->_newLineCount : 0);
    const size_t discountNewLine = (leftChild != NULL && rightChild != NULL && leftChild->_endsWithCR && rightChild->_startsWithLF);
    const bool startsWithLF = (leftChild != NULL ? leftChild->_startsWithLF : rightChild->_startsWithLF);
    const bool endsWithCR = (rightChild != NULL ? rightChild->_endsWithCR : leftChild->_endsWithCR);

    this->_init(NULL, leftChild, rightChild, len, newLineCount - (discountNewLine ? 1 : 0), startsWithLF, endsWithCR);
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
        os << "[LEAF] (len:" << this->_len << ", newLineCount:" << this->_newLineCount;
        if (this->_startsWithLF)
        {
            os << ", startsWithLF";
        }
        if (this->_endsWithCR)
        {
            os << ", endsWithCR";
        }
        os << ")" << endl;
        return;
    }

    printIndent(os, indent);
    os << "[NODE] (len:" << this->_len << ", newLineCount:" << this->_newLineCount;
    if (this->_startsWithLF)
    {
        os << ", startsWithLF";
    }
    if (this->_endsWithCR)
    {
        os << ", endsWithCR";
    }
    os << ")" << endl;

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

size_t BufferNode::getLen() const
{
    return this->_len;
}

size_t BufferNode::getNewLineCount() const
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
    BufferNode* node = start.node;
    
    if (innerNodeOffset + len <= node->_len)
    {
        // This is a simple substring
        const uint16_t *data = node->_str->getData();
        memcpy(dest, data + innerNodeOffset, sizeof(uint16_t) * len);
        return;
    }

    // uint16_t *result = new uint16_t[len];
    size_t resultOffset = 0;
    size_t remainingLen = len;
    do
    {
        const uint16_t *src = node->_str->getData();
        const size_t cnt = min(remainingLen, node->_str->getLen() - innerNodeOffset);
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

    // return shared_ptr<SimpleString>(new SimpleString(result, len));
    // return this->root->extractString(offset, len ,dest);
}

shared_ptr<String> BufferNode::getStrAt(size_t offset, size_t len)
{
    if (offset + len > this->_len)
    {
        return NULL;
    }

    BufferNode *node = this->findPieceAtOffset(offset);
    if (node == NULL)
    {
        return NULL;
    }

    return this->_getStrAt(node, offset, len);
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

        if (left->_endsWithCR && right->_startsWithLF)
        {
            // one newline is split between left and right
            // TODO
            assert(false);
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

    const size_t *lineStarts = it->_str->getLineStarts();
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
        const size_t *lineStarts = node->_str->getLineStarts();
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
            const size_t *lineStarts = node->_str->getLineStarts();
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

    // return this->_getLineIndexLength(lineIndex, node, lineStartOffset);

    // if (lineNumber < 1 || lineNumber > this->_newLineCount + 1)
    // {
    //     return false;
    // }

    // size_t searchLineIndex = lineNumber - 1;
    // size_t offset = 0;
    // size_t linesBefore = 0;
    // BufferNode *it = this;
    // while (!it->isLeaf())
    // {
    //     BufferNode *left = it->_leftChild;
    //     BufferNode *right = it->_rightChild;

    //     if (left == NULL)
    //     {
    //         // go right
    //         it = right;
    //     }

    //     if (right == NULL)
    //     {
    //         // go left
    //         it = left;
    //     }

    //     if (left->_endsWithCR && right->_startsWithLF)
    //     {
    //         // one newline is split between left and right
    //         // TODO
    //         assert(false);
    //     }

    //     if (searchLineIndex <= left->_newLineCount)
    //     {
    //         // go left
    //         it = left;
    //         continue;
    //     }

    //     // go right
    //     searchLineIndex -= left->_newLineCount;
    //     offset += left->_len;
    //     linesBefore += left->_newLineCount;
    //     it = right;
    // }

    // const size_t *lineStarts = it->_str->getLineStarts();
    // const size_t nodeOffset = (searchLineIndex == 0 ? 0 : lineStarts[searchLineIndex - 1]);

    // result.offset = offset + nodeOffset;
    // result.linesBefore = linesBefore;
    // result.node = it;
    // result.nodeOffset = nodeOffset;

    return true;
}

// bool BufferNode::findLineStart(size_t lineNumber, BufferCursor &result)
// {
//     if (lineNumber < 1 || lineNumber > this->_newLineCount + 1)
//     {
//         return false;
//     }

//     size_t searchLineIndex = lineNumber - 1;
//     size_t offset = 0;
//     size_t linesBefore = 0;
//     BufferNode *it = this;
//     while (!it->isLeaf())
//     {
//         BufferNode *left = it->_leftChild;
//         BufferNode *right = it->_rightChild;

//         if (left == NULL)
//         {
//             // go right
//             it = right;
//         }

//         if (right == NULL)
//         {
//             // go left
//             it = left;
//         }

//         if (left->_endsWithCR && right->_startsWithLF)
//         {
//             // one newline is split between left and right
//             // TODO
//             assert(false);
//         }

//         if (searchLineIndex <= left->_newLineCount)
//         {
//             // go left
//             it = left;
//             continue;
//         }

//         // go right
//         searchLineIndex -= left->_newLineCount;
//         offset += left->_len;
//         linesBefore += left->_newLineCount;
//         it = right;
//     }

//     const size_t *lineStarts = it->_str->getLineStarts();
//     const size_t nodeOffset = (searchLineIndex == 0 ? 0 : lineStarts[searchLineIndex - 1]);

//     result.offset = offset + nodeOffset;
//     result.linesBefore = linesBefore;
//     result.node = it;
//     result.nodeOffset = nodeOffset;

//     return true;
// }

// void BufferNode::moveToLineEnd(BufferCursor &cursor)
// {
//     const size_t nodeLineCount = cursor.node->_newLineCount;
//     assert(cursor.node->isLeaf());
//     // assert(nodeLineCount >= lineIndex);

//     if (lineIndex < nodeLineCount)
//     {
//         // lucky, the line ends in this same block
//         const size_t *lineStarts = node->_str->getLineStarts();
//         const size_t lineEndOffset = lineStarts[lineIndex];
//         return lineEndOffset - lineStartOffset;
//     }

//     // find the first newline or EOF
//     size_t result = node->_len - lineStartOffset;
//     do
//     {
//         // TODO: could probably optimize to not visit every leaf!!!
//         node = node->next();

//         if (node == NULL)
//         {
//             // EOF
//             break;
//         }

//         if (node->_newLineCount > 0)
//         {
//             const size_t *lineStarts = node->_str->getLineStarts();
//             result += lineStarts[0];
//             break;
//         }

//         // node does not contain newline
//         result += node->_len;

//     } while (true);

//     return result;
// }

shared_ptr<String> BufferNode::_getStrAt(BufferNode *node, size_t offset, size_t len)
{
    if (offset + len <= node->getLen())
    {
        // This is a simple substring
        return shared_ptr<String>(new BufferStringSubstring(node->_str, offset, len));
    }

    uint16_t *result = new uint16_t[len];
    size_t resultOffset = 0;
    size_t remainingLen = len;
    do
    {
        const uint16_t *src = node->_str->getData();
        const size_t cnt = min(remainingLen, node->_str->getLen() - offset);
        memcpy(result + resultOffset, src + offset, sizeof(uint16_t) * cnt);
        remainingLen -= cnt;
        resultOffset += cnt;
        offset = 0;

        if (remainingLen == 0)
        {
            break;
        }

        node = node->next();
        assert(node->isLeaf());
    } while (true);
    return shared_ptr<SimpleString>(new SimpleString(result, len));
}

BufferNode *BufferNode::findPieceAtLineIndex(size_t &lineIndex)
{
    if (lineIndex > this->_newLineCount)
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
            // go right
            it = right;
        }

        if (right == NULL)
        {
            // go left
            it = left;
        }

        if (left->_endsWithCR && right->_startsWithLF)
        {
            // one newline is split between left and right
            // TODO
            assert(false);
        }

        if (lineIndex <= left->_newLineCount)
        {
            // go left
            it = left;
            continue;
        }

        // go right
        lineIndex -= left->_newLineCount;
        it = right;
    }

    return it;
}

size_t BufferNode::getLineLength(size_t lineNumber)
{
    if (lineNumber < 1 || lineNumber > this->_newLineCount + 1)
    {
        return 0;
    }

    size_t lineIndex = lineNumber - 1;
    BufferNode *node = findPieceAtLineIndex(lineIndex);
    if (node == NULL)
    {
        return 0;
    }

    const size_t *lineStarts = node->_str->getLineStarts();
    const size_t lineStartOffset = (lineIndex == 0 ? 0 : lineStarts[lineIndex - 1]);

    return this->_getLineIndexLength(lineIndex, node, lineStartOffset);
}

size_t BufferNode::_getLineIndexLength(const size_t lineIndex, BufferNode *node, const size_t lineStartOffset)
{
    const size_t nodeLineCount = node->_newLineCount;
    assert(node->isLeaf());
    assert(nodeLineCount >= lineIndex);

    if (lineIndex < nodeLineCount)
    {
        // lucky, the line ends in this same block
        const size_t *lineStarts = node->_str->getLineStarts();
        const size_t lineEndOffset = lineStarts[lineIndex];
        return lineEndOffset - lineStartOffset;
    }

    // find the first newline or EOF
    size_t result = node->_len - lineStartOffset;
    do
    {
        // TODO: could probably optimize to not visit every leaf!!!
        node = node->next();

        if (node == NULL)
        {
            // EOF
            break;
        }

        if (node->_newLineCount > 0)
        {
            const size_t *lineStarts = node->_str->getLineStarts();
            result += lineStarts[0];
            break;
        }

        // node does not contain newline
        result += node->_len;

    } while (true);

    return result;
}

shared_ptr<String> BufferNode::getLineContent(size_t lineNumber)
{
    if (lineNumber < 1 || lineNumber > this->_newLineCount + 1)
    {
        return NULL;
    }

    size_t lineIndex = lineNumber - 1;
    BufferNode *node = findPieceAtLineIndex(lineIndex);
    if (node == NULL)
    {
        return NULL;
    }

    const size_t *lineStarts = node->_str->getLineStarts();
    const size_t lineStartOffset = (lineIndex == 0 ? 0 : lineStarts[lineIndex - 1]);
    size_t len = this->_getLineIndexLength(lineIndex, node, lineStartOffset);
    return this->_getStrAt(node, lineStartOffset, len);
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

size_t Buffer::getLen() const
{
    return this->root->getLen();
}

size_t Buffer::getLineCount() const
{
    return this->root->getNewLineCount() + 1;
}

shared_ptr<String> Buffer::getStrAt(size_t offset, size_t len)
{
    return this->root->getStrAt(offset, len);
}

void Buffer::extractString(BufferCursor start, size_t len, uint16_t *dest)
{
    this->root->extractString(start, len ,dest);
}

bool Buffer::findOffset(size_t offset, BufferCursor &result)
{
    return this->root->findOffset(offset, result);
}

bool Buffer::findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end)
{
    return this->root->findLine(lineNumber, start, end);
}

// (" << start.offset << "," << start.nodeStartOffset <<")

// bool Buffer::findLineStart(size_t lineNumber, BufferCursor &result)
// {
//     return this->root->findLineStart(lineNumber, result);
// }

// void Buffer::moveToLineEnd(BufferCursor &cursor)
// {
//     this->root->moveToLineEnd(cursor);
// }

size_t Buffer::getLineLength(size_t lineNumber)
{
    return this->root->getLineLength(lineNumber);
}

shared_ptr<String> Buffer::getLineContent(size_t lineNumber)
{
    return this->root->getLineContent(lineNumber);
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

BufferNode *buildBufferFromPieces(vector<shared_ptr<BufferString>> &pieces, size_t start, size_t end)
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

    _rawPieces.push_back(shared_ptr<BufferString>(new BufferString(data, dataLen)));
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

        _rawPieces.push_back(shared_ptr<BufferString>(new BufferString(data, dataLen)));

        return;
    }

    if (_hasPreviousChar)
    {
        _hasPreviousChar = false;
        // recreate last chunk

        shared_ptr<BufferString> lastPiece = _rawPieces[_rawPieces.size() - 1];
        size_t prevDataLen = lastPiece->getLen();
        const uint16_t *prevData = lastPiece->getData();

        size_t dataLen = prevDataLen + 1;
        uint16_t *data = new uint16_t[dataLen];
        memcpy(data, prevData, sizeof(uint16_t) * prevDataLen);
        data[dataLen - 1] = _previousChar;

        _rawPieces[_rawPieces.size() - 1] = shared_ptr<BufferString>(new BufferString(data, dataLen));
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
//     vector<shared_ptr<BufferString>> rawPieces;
//     ifs.seekg(0, std::ios::beg);
//     while (!ifs.eof())
//     {
//         char *piece = new char[PIECE_SIZE];

//         ifs.read(piece, PIECE_SIZE);

//         if (ifs)
//         {
//             rawPieces.push_back(shared_ptr<BufferString>(new BufferString(piece, PIECE_SIZE)));
//         }
//         else
//         {
//             rawPieces.push_back(shared_ptr<BufferString>(new BufferString(piece, ifs.gcount())));
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
