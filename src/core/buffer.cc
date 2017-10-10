/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "buffer.h"

#include <iostream>
#include <assert.h>
#include <cstring>

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

size_t Buffer::memUsage() const
{
    const size_t leafsCount = leafs_.length();

    size_t leafs = 0;
    for (size_t i = 0; i < leafsCount; i++)
    {
        leafs += leafs_[i]->memUsage();
    }
    return (
        sizeof(Buffer) +
        leafs_.memUsage() +
        nodesCount_ * sizeof(BufferNode) +
        leafs);
}

Buffer::Buffer(vector<BufferPiece *> &pieces, size_t minLeafLength, size_t maxLeafLength)
{
    assert(2 * minLeafLength >= maxLeafLength);

    const size_t leafsCount = pieces.size();
    BufferPiece **tmp = new BufferPiece *[leafsCount];
    for (size_t i = 0; i < leafsCount; i++)
    {
        tmp[i] = pieces[i];
    }
    leafs_.assign(tmp, leafsCount);

    nodes_ = NULL;
    _rebuildNodes();

    minLeafLength_ = minLeafLength;
    maxLeafLength_ = maxLeafLength;
    idealLeafLength_ = (minLeafLength_ + maxLeafLength_) / 2;

    // printf("mem usage: %lu B = %lf MB\n", memUsage(), ((double)memUsage()) / 1024 / 1024);
}

void Buffer::_rebuildNodes()
{
    if (nodes_ != NULL)
    {
        delete[] nodes_;
    }

    const size_t leafsCount = leafs_.length();

    nodesCount_ = 1 << log2(leafsCount);
    leafsStart_ = nodesCount_;
    leafsEnd_ = leafsStart_ + leafsCount;

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
    const size_t leafsCount = leafs_.length();
    for (size_t i = 0; i < leafsCount; i++)
    {
        delete leafs_[i];
    }
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
        const size_t cnt = min(len, leaf->length() - innerLeafOffset);
        leaf->write(dest + destOffset, innerLeafOffset, cnt);

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

#define GET_NODE_LENGTH(nodeIndex) (IS_NODE(nodeIndex) ? nodes_[nodeIndex].length : IS_LEAF(nodeIndex) ? leafs_[NODE_TO_LEAF_INDEX(nodeIndex)]->length() : 0)

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

        size_t leftLength = GET_NODE_LENGTH(left);
        size_t rightLength = GET_NODE_LENGTH(right);

        if (searchOffset < leftLength || rightLength == 0)
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

    const LINE_START_T innerLineStartOffset = (lineIndex == 0 ? 0 : leafs_[it]->lineStartFor(lineIndex - 1));

    result.offset = leafStartOffset + innerLineStartOffset;
    result.leafIndex = it;
    result.leafStartOffset = leafStartOffset;

    return true;
}

void Buffer::_findLineEnd(size_t leafIndex, size_t leafStartOffset, size_t innerLineIndex, BufferCursor &result)
{
    const size_t leafsCount = leafs_.length();
    while (true)
    {
        BufferPiece *leaf = leafs_[leafIndex];

        if (innerLineIndex < leaf->newLineCount())
        {
            LINE_START_T lineEndOffset = leafs_[leafIndex]->lineStartFor(innerLineIndex);

            result.offset = leafStartOffset + lineEndOffset;
            result.leafIndex = leafIndex;
            result.leafStartOffset = leafStartOffset;
            return;
        }

        leafIndex++;

        if (leafIndex >= leafsCount)
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

void Buffer::resolveEdits(vector<OffsetLenEdit2> &_edits, vector<InternalOffsetLenEdit2> &edits, vector<BufferString*> &toDelete)
{
    // Check if we must merge adjacent edits
    size_t mergedCnt = 1;
    for (size_t i = 1; i < _edits.size(); i++)
    {
        OffsetLenEdit2 &prevEdit = _edits[i - 1];
        OffsetLenEdit2 &edit = _edits[i];
        if (prevEdit.offset + prevEdit.length == edit.offset) {
            continue;
        }
        mergedCnt++;
    }

    vector<OffsetLenEdit2> *actual;

    vector<OffsetLenEdit2> _merged;
    if (mergedCnt != _edits.size()) {
        _merged.resize(mergedCnt);
        size_t dest = 0;

        OffsetLenEdit2 prev = _edits[0];
        for (size_t i = 1; i < _edits.size(); i++) {
            OffsetLenEdit2 curr = _edits[i];

            if (prev.offset + prev.length == curr.offset) {
                // merge into `prev`
                prev.length = prev.length + curr.length;
                BufferString* newText = BufferString::concat(prev.text, curr.text);
                toDelete.push_back(newText);
                prev.text = newText;
            } else {
                _merged[dest++] = prev;
                prev = curr;
            }
        }
        _merged[dest++] = prev;

        actual = &_merged;
    } else {
        actual = &_edits;
    }

    edits.resize(actual->size());
    BufferCursor tmp;
    for (size_t i = 0; i < actual->size(); i++)
    {
        OffsetLenEdit2 &_edit = (*actual)[i];
        InternalOffsetLenEdit2 &edit = edits[i];

        edit.text = _edit.text;

        findOffset(_edit.offset, tmp);
        edit.startLeafIndex = tmp.leafIndex;
        edit.startInnerOffset = tmp.offset - tmp.leafStartOffset;

        if (edit.startInnerOffset > 0)
        {
            BufferPiece *startLeaf = leafs_[edit.startLeafIndex];
            uint16_t charBefore = startLeaf->charAt(edit.startInnerOffset - 1);
            if (charBefore == '\r')
            {
                // include the replacement of \r in the edit
                BufferString* newText = BufferString::concat(BufferString::carriageReturn(), edit.text);
                toDelete.push_back(newText);
                edit.text = newText;

                findOffset(_edit.offset - 1, tmp);
                edit.startLeafIndex = tmp.leafIndex;
                edit.startInnerOffset = tmp.offset - tmp.leafStartOffset;
            }
        }

        findOffset(_edit.offset + _edit.length, tmp);
        edit.endLeafIndex = tmp.leafIndex;
        edit.endInnerOffset = tmp.offset - tmp.leafStartOffset;

        BufferPiece *endLeaf = leafs_[edit.endLeafIndex];
        if (edit.endInnerOffset < endLeaf->length())
        {
            uint16_t charAfter = endLeaf->charAt(edit.endInnerOffset);
            if (charAfter == '\n')
            {
                // include the replacement of \n in the edit
                BufferString* newText = BufferString::concat(edit.text, BufferString::lineFeed());
                toDelete.push_back(newText);
                edit.text = newText;

                findOffset(_edit.offset + _edit.length + 1, tmp);
                edit.endLeafIndex = tmp.leafIndex;
                edit.endInnerOffset = tmp.offset - tmp.leafStartOffset;
            }
        }
    }
}

LeafReplacement& pushLeafReplacement(size_t startLeafIndex, size_t endLeafIndex, vector<LeafReplacement> &replacements)
{
    LeafReplacement tmp;
    replacements.push_back(tmp);

    LeafReplacement &res = replacements[replacements.size() - 1];
    res.startLeafIndex = startLeafIndex;
    res.endLeafIndex = endLeafIndex;
    res.replacements = new vector<BufferPiece*>();
    return res;
}

void Buffer::flushLeafEdits(size_t accumulatedLeafIndex, vector<LeafOffsetLenEdit2> &accumulatedLeafEdits, vector<LeafReplacement> &replacements)
{
    if (accumulatedLeafEdits.size() > 0)
    {
        LeafReplacement &rep = pushLeafReplacement(accumulatedLeafIndex, accumulatedLeafIndex, replacements);
        leafs_[accumulatedLeafIndex]->replaceOffsetLen(accumulatedLeafEdits, idealLeafLength_, maxLeafLength_, rep.replacements);
    }

    accumulatedLeafEdits.clear();
}

void pushLeafEdits(size_t start, size_t length, const BufferString *text, vector<LeafOffsetLenEdit2> &accumulatedLeafEdits)
{
    if (length != 0 || text->length() != 0)
    {
        LeafOffsetLenEdit2 tmp;
        tmp.start = start;
        tmp.length = length;
        tmp.text = text;
        accumulatedLeafEdits.push_back(tmp);
    }
}

void Buffer::appendLeaf(BufferPiece *leaf, vector<BufferPiece*> &leafs, BufferPiece *&prevLeaf)
{
    if (prevLeaf == NULL)
    {
        leafs.push_back(leaf);
        prevLeaf = leaf;
        return;
    }

    size_t prevLeafLength = prevLeaf->length();
    size_t currLeafLength = leaf->length();

    if ((prevLeafLength < minLeafLength_ || currLeafLength < minLeafLength_) && prevLeafLength + currLeafLength <= maxLeafLength_)
    {
        BufferPiece *modifiedPrevLeaf = prevLeaf->join2(leaf);
        delete prevLeaf;

        leafs[leafs.size() - 1] = modifiedPrevLeaf;
        prevLeaf = modifiedPrevLeaf;

        // this leaf must be deleted
        delete leaf;
        return;
    }

    uint16_t lastChar = prevLeaf->charAt(prevLeafLength - 1);
    uint16_t firstChar = leaf->charAt(0);

    if (
        (lastChar >= 0xd800 && lastChar <= 0xdbff)
        || (lastChar == '\r' && firstChar == '\n')
    ) {
        BufferPiece *modifiedPrevLeaf = prevLeaf->deleteLastChar2();
        delete prevLeaf;

        leafs[leafs.size() - 1] = modifiedPrevLeaf;
        prevLeaf = modifiedPrevLeaf;

        BufferPiece *modifiedLeaf = leaf->insertFirstChar2(lastChar);
        delete leaf;
        leaf = modifiedLeaf;
    }

    leafs.push_back(leaf);
    prevLeaf = leaf;
}

void Buffer::replaceOffsetLen(vector<OffsetLenEdit2> &_edits)
{
    struct timespec start;

    const size_t initialLeafLength = leafs_.length();
    vector<BufferString*> toDelete;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    vector<InternalOffsetLenEdit2> edits;
    resolveEdits(_edits, edits, toDelete);
    // print_diff("    resolving edits", start);

    size_t accumulatedLeafIndex = 0;
    vector<LeafOffsetLenEdit2> accumulatedLeafEdits;
    vector<LeafReplacement> replacements;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    for (size_t i = 0, len = edits.size(); i < len; i++)
    {
        InternalOffsetLenEdit2 &edit = edits[i];

        // printf("---> replace @ [%lu,%lu] -> [%lu,%lu] with [%lu]\n", edit.startLeafIndex, edit.startInnerOffset, edit.endLeafIndex, edit.endInnerOffset, edit.text->length());

        size_t startLeafIndex = edit.startLeafIndex;
        size_t endLeafIndex = edit.endLeafIndex;

        if (startLeafIndex != accumulatedLeafIndex)
        {
            flushLeafEdits(accumulatedLeafIndex, accumulatedLeafEdits, replacements);
            accumulatedLeafIndex = startLeafIndex;
        }

        size_t leafEditStart = edit.startInnerOffset;
        size_t leafEditEnd = (startLeafIndex == endLeafIndex ? edit.endInnerOffset : leafs_[startLeafIndex]->length());
        pushLeafEdits(leafEditStart, leafEditEnd - leafEditStart, edit.text, accumulatedLeafEdits);

        if (startLeafIndex < endLeafIndex)
        {
            flushLeafEdits(accumulatedLeafIndex, accumulatedLeafEdits, replacements);
            accumulatedLeafIndex = endLeafIndex;

            // delete leafs in the middle
            if (startLeafIndex + 1 < endLeafIndex) {
                pushLeafReplacement(startLeafIndex + 1, endLeafIndex - 1, replacements);
            }

            // delete on last line
            size_t leafEditStart = 0;
            size_t leafEditEnd = edit.endInnerOffset;
            pushLeafEdits(leafEditStart, leafEditEnd - leafEditStart, BufferString::empty(), accumulatedLeafEdits);
        }
    }
    flushLeafEdits(accumulatedLeafIndex, accumulatedLeafEdits, replacements);

    for (size_t i = 0, len = toDelete.size(); i < len; i++)
    {
        delete toDelete[i];
    }
    // print_diff("    applying edits", start);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    vector<BufferPiece*> leafs;
    size_t leafIndex = 0;
    BufferPiece *prevLeaf = NULL;

    // BufferPiece *leaf = leafs_[leafIndex];
    for (size_t i = 0, len = replacements.size(); i < len; i++)
    {
        size_t replaceStartLeafIndex = replacements[i].startLeafIndex;
        size_t replaceEndLeafIndex = replacements[i].endLeafIndex;
        vector<BufferPiece*> innerLeafs = *(replacements[i].replacements);

        // add leafs to the left of this replace op.
        while (leafIndex < replaceStartLeafIndex) {
            appendLeaf(leafs_[leafIndex], leafs, prevLeaf);
            // leafs.push_back(leafs_[leafIndex]);
            leafIndex++;
        }

        // delete leafs that get replaced.
        while (leafIndex <= replaceEndLeafIndex) {
            delete leafs_[leafIndex];
            leafIndex++;
        }

        // add new leafs.
        for (size_t j = 0, lenJ = innerLeafs.size(); j < lenJ; j++)
        {
            appendLeaf(innerLeafs[j], leafs, prevLeaf);
            // leafs.push_back(innerLeafs[j]);
        }

        delete replacements[i].replacements;
    }

    // add remaining leafs to the right of the last replacement.
    while (leafIndex < initialLeafLength)
    {
        appendLeaf(leafs_[leafIndex], leafs, prevLeaf);
        // leafs.push_back(leafs_[leafIndex]);
        leafIndex++;
    }
    // print_diff("    recreating leafs", start);

    if (leafs.size() == 0) {
        // don't leave behind an empty leafs array
        uint16_t *tmp = new uint16_t[0];
        BufferPiece *tmp2 = new TwoBytesBufferPiece(tmp, 0);
        leafs.push_back(tmp2);
    }

    leafs_.assign(leafs);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    _rebuildNodes();
    // print_diff("    rebuilding nodes", start);
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

void Buffer::assertInvariants()
{
    const size_t leafsCount = leafs_.length();
    assert(leafsStart_ == nodesCount_);
    assert(leafsEnd_ == leafsStart_ + leafsCount);

    BufferPiece *prevLeafWithContent = NULL;
    for (size_t i = 0; i < leafsCount; i++)
    {
        BufferPiece *leaf = leafs_[i];
        if (leaf->length() > 0 && prevLeafWithContent != NULL)
        {
            uint16_t lastChar = prevLeafWithContent->charAt(prevLeafWithContent->length() - 1);
            uint16_t firstChar = leaf->charAt(0);

            bool lastIsHighSurrogate = (0xD800 <= lastChar && lastChar <= 0xDBFF);
            bool firstIsLowSurrogate = (0xDC00 <= firstChar && firstChar <= 0xDFFF);
            if (lastIsHighSurrogate && firstIsLowSurrogate)
            {
                assert(false);
            }

            bool lastIsCR = (lastChar == '\r');
            bool firstIsLF = (firstChar == '\n');
            if (lastIsCR && firstIsLF)
            {
                assert(false);
            }
        }
        leaf->assertInvariants();
        if (leaf->length() > 0)
        {
            prevLeafWithContent = leaf;
        }
    }

    for (size_t i = 1; i < nodesCount_; i++)
    {
        assertNodeInvariants(i);
    }
}

void Buffer::assertNodeInvariants(size_t nodeIndex)
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

    assert(nodes_[nodeIndex].length == length);
    assert(nodes_[nodeIndex].newLineCount == newLineCount);
}
}
