/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "buffer.h"
#include "mem-manager.h"

#include <iostream>
#include <assert.h>

#define PARENT(i) (i >> 1)
#define LEFT_CHILD(i) (i << 1)
#define RIGHT_CHILD(i) ((i << 1) + 1)
#define IS_NODE(i) (i < nodesCount_)
#define IS_LEAF(i) (i >= leafsStart_ && i < leafsEnd_)
#define LEAF_INDEX(i) (i - leafsStart_)

using namespace std;

namespace edcore
{

size_t computeNodeCount(size_t leafsCount)
{
    size_t totalNodes = 0;
    while (leafsCount > 0)
    {
        if (leafsCount == 1 || leafsCount == 2)
        {
            totalNodes += 1;
            break;
        }

        size_t stepNodes;
        if (leafsCount % 2 == 0)
        {
            stepNodes = leafsCount / 2;
        }
        else
        {
            stepNodes = leafsCount / 2 + 1;
        }

        totalNodes += stepNodes;
        leafsCount = stepNodes;
    }
    return totalNodes;
}

size_t log2(size_t n)
{
    size_t v = 1;
    for (size_t pow = 1;; pow++)
    {
        v = v << 1;
        if (v >= n)
        {
            return pow;
        }
    }
    return -1;
}

void printIndent2(ostream &os, int indent)
{
    for (int i = 0; i < indent; i++)
    {
        os << " ";
    }
}

Buffer::Buffer(vector<BufferNodeString *> &pieces)
{
    leafsCount_ = pieces.size();
    leafs_ = new BufferNodeString *[leafsCount_];
    for (size_t i = 0; i < leafsCount_; i++)
    {
        leafs_[i] = pieces[i];
    }

    nodesCount_ = 1 << log2(leafsCount_);

    leafsStart_ = nodesCount_;
    leafsEnd_ = leafsStart_ + leafsCount_;

    cout << "leafsCount: " << leafsCount_ << endl;
    cout << "nodesCount: " << nodesCount_ << endl;
    cout << "leafsStart_: " << leafsStart_ << endl;

    nodes_ = new BufferNode2[nodesCount_];
    memset(nodes_, 0, nodesCount_ * sizeof(nodes_[0]));

    for (size_t i = nodesCount_ - 1; i >= 1; i--)
    {
        size_t left = LEFT_CHILD(i);
        size_t right = RIGHT_CHILD(i);

        size_t length = 0;
        size_t newLineCount = 0;

        if (IS_NODE(left))
        {
            length += nodes_[left].length;
            newLineCount += nodes_[left].newLineCount;
        }
        else if (IS_LEAF(left))
        {
            length += leafs_[LEAF_INDEX(left)]->length();
            newLineCount += leafs_[LEAF_INDEX(left)]->newLinesCount();
        }

        if (IS_NODE(right))
        {
            length += nodes_[right].length;
            newLineCount += nodes_[right].newLineCount;
        }
        else if (IS_LEAF(right))
        {
            length += leafs_[LEAF_INDEX(right)]->length();
            newLineCount += leafs_[LEAF_INDEX(right)]->newLinesCount();
        }

        nodes_[i].length = length;
        nodes_[i].newLineCount = newLineCount;
    }

    // print(cout, 1, 0);
    MM_REGISTER(this);
}

Buffer::~Buffer()
{
    MM_UNREGISTER(this);
    delete this->root;
}

void Buffer::extractString2(BufferCursor2 start, size_t len, uint16_t *dest)
{
    size_t innerNodeOffset = start.offset - start.leafStartOffset;
    size_t node = start.leafIndex;

    if (innerNodeOffset + len <= leafs_[node]->length())
    {
         // This is a simple substring
        const uint16_t *data = leafs_[node]->data();
        memcpy(dest, data + innerNodeOffset, sizeof(uint16_t) * len);
        return;
    }

    size_t resultOffset = 0;
    size_t remainingLen = len;
    do
    {
        const uint16_t *src = leafs_[node]->data();
        const size_t cnt = min(remainingLen, leafs_[node]->length() - innerNodeOffset);
        memcpy(dest + resultOffset, src + innerNodeOffset, sizeof(uint16_t) * cnt);
        remainingLen -= cnt;
        resultOffset += cnt;
        innerNodeOffset = 0;

        if (remainingLen == 0)
        {
            break;
        }

        node++;
    } while (true);
}

void Buffer::extractString(BufferCursor start, size_t len, uint16_t *dest)
{
    this->root->extractString(start, len, dest);
}

bool Buffer::findOffset(size_t offset, BufferCursor &result)
{
    return this->root->findOffset(offset, result);
}

bool Buffer::findOffset2(size_t offset, BufferCursor2 &result)
{
    if (offset > nodes_[1].length)
    {
        return false;
    }

    size_t it = 1;
    size_t searchOffset = offset;
    size_t leafStartOffset = 0;
    while (!IS_LEAF(it))
    {
        size_t left = LEFT_CHILD(it);
        size_t right = RIGHT_CHILD(it);

        size_t leftLength = 0;
        if (IS_NODE(left))
        {
            leftLength = nodes_[left].length;
        }
        else if (IS_LEAF(left))
        {
            leftLength = leafs_[LEAF_INDEX(left)]->length();
        }

        if (searchOffset < leftLength)
        {
            // go left
            it = left;
        }
        else
        {
            // go right
            searchOffset -= leftLength;
            leafStartOffset += leftLength;
            it = right;
        }
    }
    it = LEAF_INDEX(it);

    result.offset = offset;
    result.leafIndex = it;
    result.leafStartOffset = leafStartOffset;

    return true;
}

bool Buffer::_findLineStart(size_t &lineIndex, BufferCursor2 &result)
{
    if (lineIndex > nodes_[1].newLineCount)
    {
        return false;
    }

    size_t it = 1;
    size_t leafStartOffset = 0;
    while (!IS_LEAF(it))
    {
        size_t left = LEFT_CHILD(it);
        size_t right = RIGHT_CHILD(it);

        size_t leftNewLineCount = 0;
        size_t leftLength = 0;
        if (IS_NODE(left))
        {
            leftNewLineCount = nodes_[left].newLineCount;
            leftLength = nodes_[left].length;
        }
        else if (IS_LEAF(left))
        {
            leftNewLineCount = leafs_[LEAF_INDEX(left)]->newLinesCount();
            leftLength = leafs_[LEAF_INDEX(left)]->length();
        }

        if (lineIndex <= leftNewLineCount)
        {
            // go left
            it = left;
            continue;
        }

        // go right
        lineIndex -= leftNewLineCount;
        leafStartOffset += leftLength;
        it = right;
    }
    it = LEAF_INDEX(it);

    const size_t *lineStarts = leafs_[it]->lineStarts();
    const size_t innerLineStartOffset = (lineIndex == 0 ? 0 : lineStarts[lineIndex - 1]);

    result.offset = leafStartOffset + innerLineStartOffset;
    result.leafIndex = it;
    result.leafStartOffset = leafStartOffset;

    return true;
}

void Buffer::_findLineEnd(size_t leafIndex, size_t leafStartOffset, size_t innerLineIndex, BufferCursor2 &result)
{
    const size_t leafLineCount = leafs_[leafIndex]->newLinesCount();

    if (innerLineIndex < leafLineCount)
    {
        // lucky, the line ends in the same leaf
        const size_t *lineStarts = leafs_[leafIndex]->lineStarts();
        size_t lineEndOffset = lineStarts[innerLineIndex];

        result.offset = leafStartOffset + lineEndOffset;
        result.leafIndex = leafIndex;
        result.leafStartOffset = leafStartOffset;
        return;
    }

    // find the first newline or EOF
    size_t offset = leafStartOffset + leafs_[leafIndex]->length();
    do
    {
        size_t next = leafIndex+1;

        if (next >= leafsCount_)
        {
            // EOF
            break;
        }

        leafStartOffset += leafs_[leafIndex]->length();
        leafIndex = next;

        if (leafs_[leafIndex]->newLinesCount() > 0)
        {
            const size_t *lineStarts = leafs_[leafIndex]->lineStarts();
            offset = leafStartOffset + lineStarts[0];
            break;
        }
        else
        {
            offset = leafStartOffset + leafs_[leafIndex]->length();
        }
    } while (true);

    result.offset = offset;
    result.leafIndex = leafIndex;
    result.leafStartOffset = leafStartOffset;
}

bool Buffer::findLine2(size_t lineNumber, BufferCursor2 &start, BufferCursor2 &end)
{
    size_t innerLineIndex = lineNumber - 1;
    if (!_findLineStart(innerLineIndex, start))
    {
        return false;
    }

    _findLineEnd(start.leafIndex, start.leafStartOffset, innerLineIndex, end);
    return true;
}

bool Buffer::findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end)
{
    return this->root->findLine(lineNumber, start, end);
}

void Buffer::print(ostream &os, size_t index, size_t indent)
{
    if (IS_LEAF(index))
    {
        printIndent2(os, indent);
        BufferNodeString *v = leafs_[LEAF_INDEX(index)];
        os << "[LEAF] (len:" << v->length() << ", newLineCount:" << v->newLinesCount() << ")" << endl;
        return;
    }

    printIndent2(os, indent);
    os << "[NODE] (len:" << nodes_[index].length << ", newLineCount:" << nodes_[index].newLineCount << ")" << endl;

    indent += 4;
    size_t left = LEFT_CHILD(index);
    if (IS_NODE(left) || IS_LEAF(left))
    {
        print(os, left, indent);
    }
    size_t right = RIGHT_CHILD(index);
    if (IS_NODE(right) || IS_LEAF(right))
    {
        print(os, right, indent);
    }
}
}
