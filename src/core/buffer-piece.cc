/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <iostream>
#include <assert.h>
#include <cstring>

#include "buffer-piece.h"

namespace edcore
{

size_t BufferPiece::memUsage() const
{
    return (
        sizeof(BufferPiece) +
        chars_.memUsage() +
        lineStarts_.memUsage());
}

BufferPiece::BufferPiece(uint16_t *data, size_t len)
{
    assert(data != NULL);

    chars_.assign(data, len);
    _rebuildLineStarts();
}

BufferPiece::BufferPiece(uint16_t *data, size_t dataLength, LINE_START_T *lineStarts, size_t lineStartsLength)
{
    chars_.assign(data, dataLength);
    lineStarts_.assign(lineStarts, lineStartsLength);
}

void BufferPiece::_rebuildLineStarts()
{
    const size_t length = chars_.length();
    const uint16_t *data = chars_.data();

    // // Do a first pass to count the number of line starts
    // size_t lineStartsCount = 0;
    // for (size_t i = 0; i < length; i++)
    // {
    //     const uint16_t chr = data[i];

    //     if (chr == '\r')
    //     {
    //         if (i + 1 < length && data[i + 1] == '\n')
    //         {
    //             // \r\n... case
    //             lineStartsCount++;
    //             i++; // skip \n
    //         }
    //         else
    //         {
    //             // \r... case
    //             lineStartsCount++;
    //         }
    //     }
    //     else if (chr == '\n')
    //     {
    //         lineStartsCount++;
    //     }
    // }

    // LINE_START_T *lineStarts = new LINE_START_T[lineStartsCount];

    // size_t dest = 0;
    // for (size_t i = 0; i < length; i++)
    // {
    //     const uint16_t chr = data[i];

    //     if (chr == '\r')
    //     {
    //         if (i + 1 < length && data[i + 1] == '\n')
    //         {
    //             // \r\n... case
    //             lineStarts[dest++] = i + 2;
    //             i++; // skip \n
    //         }
    //         else
    //         {
    //             // \r... case
    //             lineStarts[dest++] = i + 1;
    //         }
    //     }
    //     else if (chr == '\n')
    //     {
    //         lineStarts[dest++] = i + 1;
    //     }
    // }

    // lineStarts_.assign(lineStarts, lineStartsCount);




    vector<LINE_START_T> lineStarts;

    for (size_t i = 0; i < length; i++)
    {
        uint16_t chr = data[i];

        if (chr == '\r')
        {
            if (i + 1 < length && data[i + 1] == '\n')
            {
                // \r\n... case
                lineStarts.push_back(i + 2);
                i++; // skip \n
            }
            else
            {
                // \r... case
                lineStarts.push_back(i + 1);
            }
        }
        else if (chr == '\n')
        {
            lineStarts.push_back(i + 1);
        }
    }

    lineStarts_.assign(lineStarts);
}

BufferPiece::~BufferPiece()
{
}

uint16_t BufferPiece::deleteLastChar()
{
    const size_t charsLength = chars_.length();
    const size_t lineStartsLength = lineStarts_.length();

    assert(charsLength > 0);

    const uint16_t ret = chars_[charsLength - 1];

    if (lineStartsLength > 0 && lineStarts_[lineStartsLength - 1] == charsLength)
    {
        lineStarts_.deleteLast();
    }
    chars_.deleteLast();

    return ret;
}

void BufferPiece::insertFirstChar(uint16_t character)
{
    const size_t lineStartsLength = lineStarts_.length();
    const bool insertLineStart = (character == '\r' && (lineStartsLength == 0 || lineStarts_[0] != 1 || chars_[0] != '\n'));

    for (size_t i = 0; i < lineStartsLength; i++)
    {
        lineStarts_[i] += 1;
    }

    if (insertLineStart)
    {
        lineStarts_.insertFirstElement(1);
    }
    chars_.insertFirstElement(character);
}

void BufferPiece::deleteOneOffsetLen(size_t offset, size_t len)
{
    const size_t charsLength = chars_.length();
    const size_t lineStartsLength = lineStarts_.length();

    assert(offset + len <= charsLength);

    if (offset == 0 && len == charsLength)
    {
        // fast path => everything is deleted
        lineStarts_.deleteRange(0, lineStartsLength);
        chars_.deleteRange(0, charsLength);
        return;
    }

    size_t deleteLineStartsFrom = lineStartsLength;
    size_t deleteLineStartsTo = 0;
    LINE_START_T *deletingCase1 = NULL;
    for (size_t i = 0; i < lineStartsLength; i++)
    {
        LINE_START_T lineStart = lineStarts_[i];
        if (lineStart < offset)
        {
            // Entirely before deletion
            continue;
        }
        if (lineStart > offset + len + 1)
        {
            // Entirely after deletion
            lineStarts_[i] -= len;
            continue;
        }

        // Boundary: Cover the case of deleting: \r[\n...]
        if (offset == lineStart - 1 && lineStart > 1 && chars_[lineStart - 2] == '\r' && chars_[lineStart - 1] == '\n')
        {
            // The line start remains
            lineStarts_[i] -= 1;
            deletingCase1 = &(lineStarts_[i]);
            continue;
        }

        // Boundary: Cover the case of deleting: \r[...]
        if (offset == lineStart)
        {
            if (chars_[lineStart - 1] == '\r')
            {
                deletingCase1 = &(lineStarts_[i]);
            }
            continue;
        }

        // Boundary: Cover the case of deleting: \r[...]\n
        if (offset + len == lineStart - 1)
        {
            if (deletingCase1 != NULL && chars_[lineStart - 1] == '\n')
            {
                (*deletingCase1) = (*deletingCase1) + 1;
            }
            else
            {
                // The line start remains
                lineStarts_[i] -= len;
                continue;
            }
        }

        // This line start must be deleted
        deleteLineStartsFrom = min(deleteLineStartsFrom, i);
        deleteLineStartsTo = max(deleteLineStartsTo, i + 1);
    }

    if (deleteLineStartsFrom < deleteLineStartsTo)
    {
        lineStarts_.deleteRange(deleteLineStartsFrom, deleteLineStartsTo - deleteLineStartsFrom);
    }
    chars_.deleteRange(offset, len);
}

void BufferPiece::join(const BufferPiece *other)
{
    const size_t charsLength = chars_.length();
    const size_t otherCharsLength = other->chars_.length();
    const size_t otherLineStartsLength = other->lineStarts_.length();

    if (otherCharsLength == 0)
    {
        // nothing to append
        return;
    }

    for (size_t i = 0; i < otherLineStartsLength; i++)
    {
        other->lineStarts_[i] += charsLength;
    }

    lineStarts_.append(other->lineStarts_.data(), otherLineStartsLength);
    chars_.append(other->chars_.data(), otherCharsLength);
}

void BufferPiece::split(size_t idealLeafLength, vector<BufferPiece*> &dest) const
{
    const size_t lineStartCount = lineStarts_.length();
    size_t lineStartIndex = 0;
    const size_t charCount = chars_.length();
    size_t charIndex = 0;

    while(charIndex < charCount)
    {
        size_t toCharIndex;
        if (charIndex + idealLeafLength >= charCount)
        {
            toCharIndex = charCount;
        }
        else
        {
            toCharIndex = charIndex + idealLeafLength;
            uint16_t lastChar = chars_[toCharIndex - 1];
            if (lastChar == '\r' || (lastChar >= 0xd800 && lastChar <= 0xdbff)) {
                toCharIndex--;
            }
        }

        size_t pieceLineStartCount = 0;
        for (size_t i = lineStartIndex; i < lineStartCount; i++)
        {
            LINE_START_T lineStart = lineStarts_[i];
            if (lineStart > toCharIndex)
            {
                break;
            }
            pieceLineStartCount++;
            // printf("lineStart: %lu\n", lineStart);
        }
        // printf("PIECE: %lu -> %lu, pieceLineStartCount: %lu\n", charIndex, toCharIndex, pieceLineStartCount);

        LINE_START_T *pieceLineStarts = new LINE_START_T[pieceLineStartCount];
        for (size_t i = 0; i < pieceLineStartCount; i++)
        {
            LINE_START_T lineStart = lineStarts_[lineStartIndex + i];
            pieceLineStarts[i] = lineStart - charIndex;
        }

        uint16_t *pieceChars = new uint16_t[toCharIndex - charIndex];
        memcpy(pieceChars, chars_.data() + charIndex, sizeof(*pieceChars) * (toCharIndex - charIndex));

        BufferPiece *piece = new BufferPiece(pieceChars, toCharIndex - charIndex, pieceLineStarts, pieceLineStartCount);
        dest.push_back(piece);

        charIndex = toCharIndex;
        lineStartIndex += pieceLineStartCount;
    }
}

void BufferPiece::insertOneOffsetLen(size_t offset, const uint16_t *data, size_t len)
{
    const size_t lineStartsLength = lineStarts_.length();

    for (size_t i = 0; i < lineStartsLength; i++)
    {
        LINE_START_T lineStart = lineStarts_[i];
        if (lineStart < offset)
        {
            // Entirely before insertion
            continue;
        }
        if (lineStart > offset + 1)
        {
            // Entirely after insertion
            lineStarts_[i] += len;
            continue;
        }

        // TODO
        assert(false);
    }

    chars_.insert(offset, data, len);

    // printf("TODO: insertOneOffsetLen %lu (data of %lu)\n", offset, len);
}

void BufferPiece::replaceOffsetLen(vector<LeafOffsetLenEdit> &edits)
{
    // for (size_t i = 0; i < edits.size(); i++)
    // {
    //     LeafOffsetLenEdit &edit = edits[i];
    //     printf("~~~~leaf edit: %lu,%lu -> [%lu]\n", edit.start, edit.length, edit.dataLength);
    // }

    // Determine if line starts need to be recreated (i.e. for the complicated cases)
    bool recreateLineStarts = false;
    for (size_t i = 0, len = edits.size(); i < len; i++)
    {
        LeafOffsetLenEdit &edit = edits[i];
        const size_t editStart = edit.start;

        // check if the edit has a \r before its start point
        if (editStart > 0 && chars_[editStart - 1] == '\r')
        {
            recreateLineStarts = true;
            break;
        }

        // check if the edit has a \r before its end point
        const size_t editEnd = edit.start + edit.length;
        if (editEnd > 0 && chars_[editEnd - 1] == '\r')
        {
            recreateLineStarts = true;
            break;
        }

        // check if the edit introduces a final \r
        if (edit.dataLength > 0 && edit.data[edit.dataLength - 1] == '\r')
        {
            recreateLineStarts = true;
            break;
        }
    }

    long delta = 0;
    for (size_t i1 = edits.size(); i1 > 0; i1--)
    {
        LeafOffsetLenEdit &edit = edits[i1 - 1];

        edit.resultStart = edit.start + delta;
        if (edit.dataLength > edit.length)
        {
            size_t longerBy = (edit.dataLength - edit.length);
            delta += longerBy;
        }
        else
        {
            size_t shorterBy = (edit.length - edit.dataLength);
            delta -= shorterBy;
        }
    }
    const size_t newLength = chars_.length() + delta;

    if (!_tryApplyEditsNoAllocate(edits, newLength))
    {
        _applyEditsAllocate(edits, newLength);
    }

    if (recreateLineStarts)
    {
        _rebuildLineStarts();
    }
    else
    {
        vector<LINE_START_T> lineStarts;

        size_t lineStartCount = lineStarts_.length();
        size_t lineStartIndex = 0;

        long delta = 0;
        for (size_t i1 = edits.size(); i1 > 0; i1--)
        {
            LeafOffsetLenEdit &edit = edits[i1 - 1];

            // Handle line starts before the edit
            while (lineStartIndex < lineStartCount && lineStarts_[lineStartIndex] <= edit.start)
            {
                lineStarts.push_back(lineStarts_[lineStartIndex] + delta);
                lineStartIndex++;
            }

            // Handle line starts deleted by the edit
            while (lineStartIndex < lineStartCount && lineStarts_[lineStartIndex] <= edit.start + edit.length)
            {
                lineStartIndex++;
            }

            for (size_t dataIndex = 0, dataLength = edit.dataLength; dataIndex < dataLength; dataIndex++)
            {
                uint16_t chr = edit.data[dataIndex];

                if (chr == '\r')
                {
                    if (dataIndex + 1 < dataLength && edit.data[dataIndex + 1] == '\n')
                    {
                        // \r\n... case
                        lineStarts.push_back(edit.resultStart + dataIndex + 2);
                        dataIndex++; // skip \n
                    }
                    else
                    {
                        // \r... case
                        lineStarts.push_back(edit.resultStart + dataIndex + 1);
                    }
                }
                else if (chr == '\n')
                {
                    lineStarts.push_back(edit.resultStart + dataIndex + 1);
                }
            }

            // printf("~~~~leaf edit: %lu,%lu -> [%lu] -- final start will be %lu\n", edit.start, edit.length, edit.dataLength, edit.resultStart);
            // long delta;
            if (edit.dataLength > edit.length)
            {
                size_t longerBy = (edit.dataLength - edit.length);
                delta += longerBy;
            }
            else
            {
                size_t shorterBy = (edit.length - edit.dataLength);
                delta -= shorterBy;
            }
        }

        while (lineStartIndex < lineStartCount)
        {
            LINE_START_T lineStart = lineStarts_[lineStartIndex];
            lineStarts.push_back(lineStart + delta);
            lineStartIndex++;
        }

        lineStarts_.assign(lineStarts);
    }
}

struct MemMoveOp
{
    size_t origStart;
    size_t destStart;
    size_t origEnd;
    size_t destEnd;

    void set(size_t origStart, size_t destStart, size_t count)
    {
        this->origStart = origStart;
        this->origEnd = origStart + count;
        this->destStart = destStart;
        this->destEnd = destStart + count;
    }
};
typedef struct MemMoveOp MemMoveOp;

void _applyMemMove(uint16_t *data, MemMoveOp &move)
{
    size_t cnt = move.destEnd - move.destStart;
    if (cnt != 0)
    {
        memmove(data + move.destStart, data + move.origStart, sizeof(*data) * cnt);
    }
}

bool _tryOrExecuteEditsInline(uint16_t *data, vector<MemMoveOp> &moves, bool execute)
{
    size_t startIndex = 0;
    size_t lastIndex = moves.size() - 1;
    while (startIndex < lastIndex)
    {
        // Try to consume `startIndex`
        MemMoveOp &start = moves[startIndex];
        if (start.origStart == start.origEnd)
        {
            // no-op
            startIndex++;
            continue;
        }

        MemMoveOp &next = moves[startIndex + 1];
        if (start.destEnd <= next.origStart)
        {
            // Consume startIndex
            if (execute)
            {
                _applyMemMove(data, start);
            }
            startIndex++;
            continue;
        }

        // Try to consume `lastIndex`
        MemMoveOp &last = moves[lastIndex];
        if (last.origStart == last.origEnd)
        {
            // no-op
            lastIndex--;
            continue;
        }

        MemMoveOp &prev = moves[lastIndex - 1];
        if (last.destStart >= prev.origEnd)
        {
            // Consume lastIndex
            if (execute)
            {
                _applyMemMove(data, last);
            }
            lastIndex--;
            continue;
        }

        // Cannot execute these edits inline
        return false;
    }

    if (execute)
    {
        _applyMemMove(data, moves[startIndex]);
    }

    // Can execute these edits inline
    return true;
}

bool BufferPiece::_tryApplyEditsNoAllocate(vector<LeafOffsetLenEdit> &edits, size_t newLength)
{
    if (newLength > chars_.capacity())
    {
        return false;
    }
    const size_t editsSize = edits.size();

    // Plan our memmoves
    vector<MemMoveOp> moves(editsSize + 1);

    size_t toIndex = chars_.length();
    for (size_t i = 0; i < editsSize; i++)
    {
        LeafOffsetLenEdit &edit = edits[i];
        // copy the chars that survive to the right of this edit
        size_t fromIndex = edit.start + edit.length;
        moves[editsSize - i].set(fromIndex, edit.resultStart + edit.dataLength, toIndex - fromIndex);
        toIndex = edit.start;
    }
    // copy the chars that survive to the left of the first edit
    moves[0].set(0, 0, toIndex);

    if (!_tryOrExecuteEditsInline(NULL, moves, false))
    {
        // Cannot execute edits inline
        return false;
    }

    uint16_t *data = chars_.data();
    _tryOrExecuteEditsInline(data, moves, true);

    // insert the new text
    for (size_t i = 0; i < editsSize; i++)
    {
        LeafOffsetLenEdit &edit = edits[i];
        // copy the chars that are introduced by this edit
        if (edit.dataLength > 0)
        {
            memcpy(data + edit.resultStart, edit.data, sizeof(*data) * edit.dataLength);
        }
    }

    chars_.setLength(newLength);
    return true;
}

void BufferPiece::_applyEditsAllocate(vector<LeafOffsetLenEdit> &edits, size_t newLength)
{
    uint16_t *target = new uint16_t[newLength];

    size_t originalFromIndex = 0;
    for (size_t i1 = edits.size(); i1 > 0; i1--)
    {
        LeafOffsetLenEdit &edit = edits[i1 - 1];

        // printf("~~~~leaf edit: %lu,%lu -> [%lu] -- final start will be %lu\n", edit.start, edit.length, edit.dataLength, edit.resultStart);

        // copy the chars that survive to the left of this edit
        size_t originalToIndex = edit.start;
        size_t originalCount = originalToIndex - originalFromIndex;
        if (originalCount > 0)
        {
            memcpy(target + edit.resultStart - originalCount, chars_.data() + originalFromIndex, sizeof(uint16_t) * originalCount);
        }
        originalFromIndex = edit.start + edit.length;

        // copy the chars that are introduced by this edit
        if (edit.dataLength > 0)
        {
            memcpy(target + edit.resultStart, edit.data, sizeof(uint16_t) * edit.dataLength);
        }
    }
    // copy the chars that survive to the right of the last edit
    size_t originalToIndex = chars_.length();
    size_t originalCount = originalToIndex - originalFromIndex;
    if (originalCount > 0)
    {
        memcpy(target + newLength - originalCount, chars_.data() + originalFromIndex, sizeof(uint16_t) * originalCount);
    }

    chars_.assign(target, newLength);
}

class BufferPieceString : public BufferString
{
public:
    BufferPieceString(const BufferPiece *target) { target_ = target; }
    size_t length() const { return target_->length(); }
    void write(uint16_t *buffer, size_t start, size_t length) const {
        assert(start + length <= target_->length());
        const uint16_t *src = target_->data();
        memcpy(buffer, src + start, sizeof(*buffer) * length);
    }
    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const { assert(false); /* TODO! */ }
    bool isOneByte() const { return false; /* TODO! */ }
    bool containsOnlyOneByte() const { return false; /* TODO! */ }
private:
    const BufferPiece *target_;
};

BufferString * recordString(BufferString *str, size_t index, vector<BufferString*> &toDelete)
{
    toDelete[index] = str;
    return str;
}

void BufferPiece::replaceOffsetLen(vector<LeafOffsetLenEdit2> &edits, size_t idealLeafLength, size_t maxLeafLength, vector<BufferPiece*>* result) const
{
    const size_t editsSize = edits.size();
    assert(editsSize > 0);

    const size_t originalCharsLength = chars_.length();
    if (editsSize == 1 && edits[0].text->length() == 0 && edits[0].start == 0 && edits[0].length == originalCharsLength)
    {
        // special case => deleting everything
        return;
    }

    vector<BufferString*> toDelete(editsSize + 2);
    BufferString *myString = recordString(new BufferPieceString(this), 0, toDelete);

    vector<const BufferString *> pieces(2 * editsSize + 1);
    size_t originalFromIndex = 0;
    size_t piecesTextLength = 0;
    for (size_t i = 0; i < editsSize; i++)
    {
        LeafOffsetLenEdit2 &edit = edits[i];
        // printf("~~~~leaf edit: %lu,%lu -> [%lu ~ ", edit.start, edit.length, edit.text->length());
        // edit.text->print();
        // printf("]\n");

        // maintain the chars that survive to the left of this edit
        BufferString *originalText = recordString(BufferString::substr(myString, originalFromIndex, edit.start - originalFromIndex), i + 1, toDelete);
        pieces[2*i] = originalText;
        piecesTextLength += originalText->length();

        originalFromIndex = edit.start + edit.length;
        pieces[2*i+1] = edit.text;
        piecesTextLength += edit.text->length();
    }

    // maintain the chars that survive to the right of the last edit
    BufferString *text = recordString(BufferString::substr(myString, originalFromIndex, originalCharsLength - originalFromIndex), editsSize + 1, toDelete);
    pieces[2 * editsSize] = text;
    piecesTextLength += text->length();

    size_t targetDataLength = piecesTextLength > maxLeafLength ? idealLeafLength : piecesTextLength;
    size_t targetDataOffset = 0;
    uint16_t *targetData = new uint16_t[targetDataLength];

    for (size_t pieceIndex = 0, pieceCount = pieces.size(); pieceIndex < pieceCount; pieceIndex++)
    {
        const BufferString *pieceText = pieces[pieceIndex];
        const size_t pieceLength = pieceText->length();
        if (pieceLength == 0)
        {
            continue;
        }

        size_t pieceOffset = 0;
        while (pieceOffset < pieceLength)
        {
            if (targetDataOffset >= targetDataLength)
            {
                result->push_back(new BufferPiece(targetData, targetDataLength));

                targetDataLength = piecesTextLength > maxLeafLength ? idealLeafLength : piecesTextLength;
                targetDataOffset = 0;
                targetData = new uint16_t[targetDataLength];
            }
            size_t writingCnt = min(pieceLength - pieceOffset, targetDataLength - targetDataOffset);
            pieceText->write(targetData + targetDataOffset, pieceOffset, writingCnt);

            pieceOffset += writingCnt;
            targetDataOffset += writingCnt;
            piecesTextLength -= writingCnt;

            // check that the buffer piece does not end in a \r or high surrogate
            if (targetDataOffset == targetDataLength && piecesTextLength > 0)
            {
                uint16_t lastChar = targetData[targetDataLength - 1];
                if (lastChar == '\r' || (0xD800 <= lastChar && lastChar <= 0xDBFF))
                {
                    // move lastChar over to next buffer piece
                    targetDataLength -= 1;
                    pieceOffset -= 1;
                    targetDataOffset -= 1;
                    piecesTextLength += 1;
                }
            }
        }
    }

    result->push_back(new BufferPiece(targetData, targetDataLength));

    for (size_t i = 0, len = toDelete.size(); i < len; i++)
    {
        delete toDelete[i];
    }
}

void BufferPiece::assertInvariants()
{
    const size_t charsLength = chars_.length();
    const size_t lineStartsLength = lineStarts_.length();

    assert(chars_.data() != NULL);
    assert(charsLength <= chars_.capacity());

    assert(lineStarts_.data() != NULL);
    assert(lineStartsLength <= lineStarts_.capacity());

    for (size_t i = 0; i < lineStartsLength; i++)
    {
        LINE_START_T lineStart = lineStarts_[i];

        assert(lineStart > 0 && lineStart <= charsLength);

        if (i > 0)
        {
            LINE_START_T prevLineStart = lineStarts_[i - 1];
            assert(lineStart > prevLineStart);
        }

        uint16_t charBefore = chars_[lineStart - 1];
        assert(charBefore == '\n' || charBefore == '\r');

        if (charBefore == '\r' && lineStart < charsLength)
        {
            uint16_t charAfter = chars_[lineStart];
            assert(charAfter != '\n');
        }
    }
}

struct timespec time_diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

void print_diff(const char *pre, struct timespec start)
{
    struct timespec end;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

    struct timespec diff = time_diff(start, end);
    size_t ns = 1000000000 * diff.tv_sec + diff.tv_nsec;

    printf("%s", pre);
    printf(": %lu nanoseconds i.e. %lf ms\n", ns, ((double)ns) / 1000000);
}
}
