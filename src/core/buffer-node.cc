/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "buffer-node.h"
#include "mem-manager.h"

#include <assert.h>

using namespace std;


namespace edcore
{

void printIndent(ostream &os, int indent);


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

void printIndent(ostream &os, int indent)
{
    for (int i = 0; i < indent; i++)
    {
        os << " ";
    }
}
}
