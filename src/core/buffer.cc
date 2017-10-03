/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "buffer.h"

#include <iostream>
#include <assert.h>

#define PARENT(i) (i >> 1)
#define LEFT_CHILD(i) (i << 1)
#define RIGHT_CHILD(i) ((i << 1) + 1)
#define IS_NODE(i) (i < nodesCount_)
#define IS_LEAF(i) (i >= leafsStart_ && i < leafsEnd_)
#define NODE_TO_LEAF_INDEX(i) (i - leafsStart_)
#define LEAF_TO_NODE_INDEX(i) (i + leafsStart_)

using namespace std;

namespace edcore
{

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

void printIndent(ostream &os, int indent)
{
    for (int i = 0; i < indent; i++)
    {
        os << " ";
    }
}

Buffer::Buffer(vector<BufferPiece *> &pieces)
{
    leafsCount_ = pieces.size();
    leafs_ = new BufferPiece *[leafsCount_];
    for (size_t i = 0; i < leafsCount_; i++)
    {
        leafs_[i] = pieces[i];
    }

    nodesCount_ = 1 << log2(leafsCount_);

    leafsStart_ = nodesCount_;
    leafsEnd_ = leafsStart_ + leafsCount_;

    nodes_ = new BufferNode[nodesCount_];
    memset(nodes_, 0, nodesCount_ * sizeof(nodes_[0]));

    for (size_t i = nodesCount_ - 1; i >= 1; i--)
    {
        _updateSingleNode(i);
    }
}

void Buffer::_updateSingleNode(size_t nodeIndex)
{
    size_t left = LEFT_CHILD(nodeIndex);
    size_t right = RIGHT_CHILD(nodeIndex);

    size_t length = 0;
    size_t newLineCount = 0;

    if (IS_NODE(left))
    {
        length += nodes_[left].length;
        newLineCount += nodes_[left].newLineCount;
    }
    else if (IS_LEAF(left))
    {
        length += leafs_[NODE_TO_LEAF_INDEX(left)]->length();
        newLineCount += leafs_[NODE_TO_LEAF_INDEX(left)]->newLineCount();
    }

    if (IS_NODE(right))
    {
        length += nodes_[right].length;
        newLineCount += nodes_[right].newLineCount;
    }
    else if (IS_LEAF(right))
    {
        length += leafs_[NODE_TO_LEAF_INDEX(right)]->length();
        newLineCount += leafs_[NODE_TO_LEAF_INDEX(right)]->newLineCount();
    }

    nodes_[nodeIndex].length = length;
    nodes_[nodeIndex].newLineCount = newLineCount;
}

Buffer::~Buffer()
{
    delete[] nodes_;
    for (size_t i = 0; i < leafsCount_; i++)
    {
        delete leafs_[i];
    }
    delete[] leafs_;
}

void Buffer::extractString(BufferCursor start, size_t len, uint16_t *dest)
{
    assert(start.offset + len <= nodes_[1].length);

    size_t innerLeafOffset = start.offset - start.leafStartOffset;
    size_t leafIndex = start.leafIndex;
    size_t destOffset = 0;
    while (len > 0)
    {
        BufferPiece *leaf = leafs_[leafIndex];
        const uint16_t *src = leaf->data();
        const size_t cnt = min(len, leaf->length() - innerLeafOffset);
        memcpy(dest + destOffset, src + innerLeafOffset, sizeof(uint16_t) * cnt);
        len -= cnt;
        destOffset += cnt;
        innerLeafOffset = 0;

        if (len == 0)
        {
            break;
        }

        leafIndex++;
    }
}

bool Buffer::findOffset(size_t offset, BufferCursor &result)
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
            leftLength = leafs_[NODE_TO_LEAF_INDEX(left)]->length();
        }

        if (searchOffset < leftLength || !(IS_NODE(right) || IS_LEAF(right)))
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
    it = NODE_TO_LEAF_INDEX(it);

    result.offset = offset;
    result.leafIndex = it;
    result.leafStartOffset = leafStartOffset;

    return true;
}

bool Buffer::_findLineStart(size_t &lineIndex, BufferCursor &result)
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
            leftNewLineCount = leafs_[NODE_TO_LEAF_INDEX(left)]->newLineCount();
            leftLength = leafs_[NODE_TO_LEAF_INDEX(left)]->length();
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
    it = NODE_TO_LEAF_INDEX(it);

    const LINE_START_T *lineStarts = leafs_[it]->lineStarts();
    const LINE_START_T innerLineStartOffset = (lineIndex == 0 ? 0 : lineStarts[lineIndex - 1]);

    result.offset = leafStartOffset + innerLineStartOffset;
    result.leafIndex = it;
    result.leafStartOffset = leafStartOffset;

    return true;
}

void Buffer::_findLineEnd(size_t leafIndex, size_t leafStartOffset, size_t innerLineIndex, BufferCursor &result)
{
    while (true)
    {
        BufferPiece *leaf = leafs_[leafIndex];

        if (innerLineIndex < leaf->newLineCount())
        {
            const LINE_START_T *lineStarts = leafs_[leafIndex]->lineStarts();
            LINE_START_T lineEndOffset = lineStarts[innerLineIndex];

            result.offset = leafStartOffset + lineEndOffset;
            result.leafIndex = leafIndex;
            result.leafStartOffset = leafStartOffset;
            return;
        }

        leafIndex++;

        if (leafIndex >= leafsCount_)
        {
            result.offset = leafStartOffset + leaf->length();
            result.leafIndex = leafIndex - 1;
            result.leafStartOffset = leafStartOffset;
            return;
        }

        leafStartOffset += leaf->length();
        innerLineIndex = 0;
    }
}

bool Buffer::findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end)
{
    size_t innerLineIndex = lineNumber - 1;
    if (!_findLineStart(innerLineIndex, start))
    {
        return false;
    }

    _findLineEnd(start.leafIndex, start.leafStartOffset, innerLineIndex, end);
    return true;
}

void Buffer::deleteOneOffsetLen(size_t offset, size_t len)
{
    assert(offset + len <= nodes_[1].length);

    BufferCursor start;
    if (!findOffset(offset, start))
    {
        assert(false);
        return;
    }

    size_t innerLeafOffset = start.offset - start.leafStartOffset;
    size_t leafIndex = start.leafIndex;
    while (len > 0)
    {
        BufferPiece *leaf = leafs_[leafIndex];
        const size_t cnt = min(len, leaf->length() - innerLeafOffset);
        leaf->deleteOneOffsetLen(innerLeafOffset, cnt);
        len -= cnt;
        innerLeafOffset = 0;

        if (len == 0)
        {
            break;
        }

        leafIndex++;
    }

    // size_t nextLeafIndex =

    // Maintain invariant that a leaf does not end in \r or a high surrogate pair
    size_t firstDirtyIndex = start.leafIndex;
    size_t lastDirtyIndex = leafIndex;
    {
        BufferPiece *startLeaf = leafs_[start.leafIndex];
        size_t startLeafLength = startLeaf->length();
        if (startLeafLength > 0)
        {
            uint16_t lastChar = startLeaf->data()[startLeafLength - 1];
            if (lastChar == 13 || (lastChar >= 0xd800 && lastChar <= 0xdbff))
            {
                size_t nextLeafIndex = leafIndex > start.leafIndex ? leafIndex : start.leafIndex + 1;
                BufferPiece *nextLeaf = NULL;
                while (nextLeafIndex < leafsCount_)
                {
                    nextLeaf = leafs_[nextLeafIndex];
                    if (nextLeaf->length() > 0)
                    {
                        break;
                    }
                    nextLeafIndex++;
                }

                if (nextLeaf != NULL && nextLeaf->length() > 0)
                {
                    startLeaf->deleteLastChar();
                    nextLeaf->insertFirstChar(lastChar);
                    lastDirtyIndex = nextLeafIndex;
                    // printf("TODO: I need to insert the last character!\n");
                }
            }
        }
    }
    // if (lastChar == 13 || (lastChar >= 0xd800 && lastChar <= 0xdbff))
    //         if (start.leafIndex < leafIndex || start.leafIndex + 1 < leafsCount_)
    //         {
    //             startLeaf->deleteLastChar();
    //             size_t nextLeafIndex = (start.leafIndex)
    //                 printf("TODO: I need to move the last character!\n");
    //         }
    //     }
    //     // uint16_t lastChar = startLeaf->data()
    //     // size_t nextLeaf = (start.leafIndex)
    // }
    // size_t startLeafLength = leafs_[start.leafIndex]->length();
    // if (leafs_[start.leafIndex]->length() > 0)

    size_t fromNodeIndex = LEAF_TO_NODE_INDEX(firstDirtyIndex) / 2;
    size_t toNodeIndex = LEAF_TO_NODE_INDEX(lastDirtyIndex) / 2;
    _updateNodes(fromNodeIndex, toNodeIndex);
}

void Buffer::_updateNodes(size_t fromNodeIndex, size_t toNodeIndex)
{
    while (fromNodeIndex != 0)
    {
        for (size_t nodeIndex = fromNodeIndex; nodeIndex <= toNodeIndex; nodeIndex++)
        {
            _updateSingleNode(nodeIndex);
        }
        fromNodeIndex /= 2;
        toNodeIndex /= 2;
    }
}

void Buffer::print(ostream &os, size_t index, size_t indent)
{
    if (IS_LEAF(index))
    {
        printIndent(os, indent);
        BufferPiece *v = leafs_[NODE_TO_LEAF_INDEX(index)];
        os << "[LEAF] (len:" << v->length() << ", newLineCount:" << v->newLineCount() << ")" << endl;
        return;
    }

    printIndent(os, indent);
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
