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

BufferNode::BufferNode(BufferNodeString* str)
{
    assert(str != NULL);
    this->_init(str, NULL, NULL, str->length(), str->newLinesCount());
}

BufferNode::BufferNode(BufferNode *leftChild, BufferNode *rightChild)
{
    assert(leftChild != NULL || rightChild != NULL);

    const size_t len = (leftChild != NULL ? leftChild->length_ : 0) + (rightChild != NULL ? rightChild->length_ : 0);
    const size_t newLineCount = (leftChild != NULL ? leftChild->newLineCount_ : 0) + (rightChild != NULL ? rightChild->newLineCount_ : 0);

    this->_init(NULL, leftChild, rightChild, len, newLineCount);
}

void BufferNode::_init(BufferNodeString* str, BufferNode *leftChild, BufferNode *rightChild, size_t len, size_t newLineCount)
{
    this->str_ = str;
    this->leftChild_ = leftChild;
    this->rightChild_ = rightChild;
    this->parent_ = NULL;
    this->length_ = len;
    this->newLineCount_ = newLineCount;
    MM_REGISTER(this);
}

BufferNode::~BufferNode()
{
    MM_UNREGISTER(this);
    if (this->str_ != NULL)
    {
        delete this->str_;
        this->str_ = NULL;
    }
    if (this->leftChild_ != NULL)
    {
        delete this->leftChild_;
        this->leftChild_ = NULL;
    }
    if (this->rightChild_ != NULL)
    {
        delete this->rightChild_;
        this->rightChild_ = NULL;
    }
    this->parent_ = NULL;
}

void BufferNode::print(ostream &os)
{
    this->_log(os, 0);
}

void BufferNode::_log(ostream &os, int indent)
{
    if (this->isLeaf())
    {
        printIndent(os, indent);
        os << "[LEAF] (len:" << this->length_ << ", newLineCount:" << this->newLineCount_ << ")" << endl;
        return;
    }

    printIndent(os, indent);
    os << "[NODE] (len:" << this->length_ << ", newLineCount:" << this->newLineCount_ << ")" << endl;

    indent += 4;
    if (this->leftChild_)
    {
        this->leftChild_->_log(os, indent);
    }
    if (this->rightChild_)
    {
        this->rightChild_->_log(os, indent);
    }
}

BufferNode *BufferNode::firstLeaf()
{
    // TODO: this will not work for an unbalanced tree
    BufferNode *res = this;
    while (res->leftChild_ != NULL)
    {
        res = res->leftChild_;
    }
    return res;
}

BufferNode *BufferNode::next()
{
    assert(this->isLeaf());
    if (this->parent_ == NULL) {
        return NULL;
    }
    if (this->parent_->leftChild_ == this)
    {
        BufferNode *sibling = this->parent_->rightChild_;
        return sibling->firstLeaf();
    }

    BufferNode *it = this;
    while (it->parent_ != NULL && it->parent_->rightChild_ == it)
    {
        it = it->parent_;
    }
    if (it->parent_ == NULL)
    {
        // EOF
        return NULL;
    }
    return it->parent_->rightChild_->firstLeaf();
}

void BufferNode::extractString(BufferCursor start, size_t len, uint16_t *dest)
{
    size_t innerNodeOffset = start.offset - start.nodeStartOffset;
    BufferNode *node = start.node;

    if (innerNodeOffset + len <= node->length_)
    {
        // This is a simple substring
        const uint16_t *data = node->str_->data();
        memcpy(dest, data + innerNodeOffset, sizeof(uint16_t) * len);
        return;
    }

    size_t resultOffset = 0;
    size_t remainingLen = len;
    do
    {
        const uint16_t *src = node->str_->data();
        const size_t cnt = min(remainingLen, node->str_->length() - innerNodeOffset);
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
    if (offset > this->length_)
    {
        return false;
    }

    BufferNode *it = this;
    size_t searchOffset = offset;
    size_t nodeStartOffset = 0;
    while (!it->isLeaf())
    {
        BufferNode *left = it->leftChild_;
        BufferNode *right = it->rightChild_;

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

        if (searchOffset < left->length_)
        {
            // go left
            it = left;
            continue;
        }

        // go right
        searchOffset -= left->length_;
        nodeStartOffset += left->length_;
        it = right;
    }

    result.offset = offset;
    result.node = it;
    result.nodeStartOffset = nodeStartOffset;

    return true;
}

bool BufferNode::_findLineStart(size_t &lineIndex, BufferCursor &result)
{
    if (lineIndex > this->newLineCount_)
    {
        return false;
    }

    BufferNode *it = this;
    size_t nodeStartOffset = 0;
    while (!it->isLeaf())
    {
        BufferNode *left = it->leftChild_;
        BufferNode *right = it->rightChild_;

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

        if (lineIndex <= left->newLineCount_)
        {
            // go left
            it = left;
            continue;
        }

        // go right
        lineIndex -= left->newLineCount_;
        nodeStartOffset += left->length_;
        it = right;
    }

    const size_t *lineStarts = it->str_->lineStarts();
    const size_t innerLineStartOffset = (lineIndex == 0 ? 0 : lineStarts[lineIndex - 1]);

    result.offset = nodeStartOffset + innerLineStartOffset;
    result.node = it;
    result.nodeStartOffset = nodeStartOffset;

    return true;
}

void BufferNode::_findLineEnd(BufferNode *node, size_t nodeStartOffset, size_t innerLineIndex, BufferCursor &result)
{
    const size_t nodeLineCount = node->newLineCount_;
    assert(node->isLeaf());
    assert(nodeLineCount >= innerLineIndex);

    if (innerLineIndex < nodeLineCount)
    {
        // lucky, the line ends in this same node
        const size_t *lineStarts = node->str_->lineStarts();
        size_t lineEndOffset = lineStarts[innerLineIndex];

        result.offset = nodeStartOffset + lineEndOffset;
        result.node = node;
        result.nodeStartOffset = nodeStartOffset;
        return;
    }

    // find the first newline or EOF
    size_t offset = nodeStartOffset + node->length_;
    do
    {
        // TODO: could probably optimize to not visit every leaf!!!
        BufferNode *next = node->next();

        if (next == NULL)
        {
            // EOF
            break;
        }

        nodeStartOffset += node->length_;
        node = next;

        if (node->newLineCount_ > 0)
        {
            const size_t *lineStarts = node->str_->lineStarts();
            offset = nodeStartOffset + lineStarts[0];
            break;
        }
        else
        {
            offset = nodeStartOffset + node->length_;
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
