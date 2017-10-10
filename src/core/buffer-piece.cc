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

BufferPiece *BufferPiece::createFromString(const BufferString *str)
{
    const size_t strLength = str->length();

    if (str->containsOnlyOneByte())
    {
        uint8_t *oneByteData = new uint8_t[strLength];
        str->writeOneByte(oneByteData, 0, strLength);
        return new OneByteBufferPiece(oneByteData, strLength);
    }
    else
    {
        uint16_t *twoByteData = new uint16_t[strLength];
        str->write(twoByteData, 0, strLength);
        return new TwoByteBufferPiece(twoByteData, strLength);
    }
}

BufferPiece *BufferPiece::deleteLastChar2(const BufferPiece *target)
{
    const size_t targetCharsLength = target->length();
    const size_t targetLineStartsLength = target->newLineCount();
    const LINE_START_T *targetLineStarts = target->lineStarts();

    size_t newLineStartsLength;
    if (targetLineStartsLength > 0 && targetLineStarts[targetLineStartsLength - 1] == targetCharsLength)
    {
        newLineStartsLength = targetLineStartsLength - 1;
    }
    else
    {
        newLineStartsLength = targetLineStartsLength;
    }

    LINE_START_T *newLineStarts = new LINE_START_T[newLineStartsLength];
    memcpy(newLineStarts, targetLineStarts, sizeof(*newLineStarts) * newLineStartsLength);

    const size_t newCharsLength = targetCharsLength - 1;
    if (target->isOneByte())
    {
        uint8_t *newData = new uint8_t[newCharsLength];
        target->writeOneByte(newData, 0, newCharsLength);
        return new OneByteBufferPiece(newData, newCharsLength, newLineStarts, newLineStartsLength);
    }
    else
    {
        uint16_t *newData = new uint16_t[newCharsLength];
        target->write(newData, 0, newCharsLength);
        return new TwoByteBufferPiece(newData, newCharsLength, newLineStarts, newLineStartsLength);
    }
}

BufferPiece *BufferPiece::insertFirstChar2(const BufferPiece *target, uint16_t character)
{
    const size_t targetCharsLength = target->length();
    const size_t targetLineStartsLength = target->newLineCount();
    const LINE_START_T *targetLineStarts = target->lineStarts();
    const bool insertLineStart = ((character == '\r' && (targetLineStartsLength == 0 || targetLineStarts[0] != 1 || target->charAt(0) != '\n')) || (character == '\n'));


    const size_t newLineStartsLength = (insertLineStart ? targetLineStartsLength + 1 : targetLineStartsLength);
    LINE_START_T *newLineStarts = new LINE_START_T[newLineStartsLength];

    if (insertLineStart)
    {
        newLineStarts[0] = 1;
        for (size_t i = 0; i < targetLineStartsLength; i++)
        {
            newLineStarts[i + 1] = targetLineStarts[i] + 1;
        }
    }
    else
    {
        for (size_t i = 0; i < targetLineStartsLength; i++)
        {
            newLineStarts[i] = targetLineStarts[i] + 1;
        }
    }

    const size_t newCharsLength = targetCharsLength + 1;
    if (target->isOneByte() && character < 256)
    {
        uint8_t *newData = new uint8_t[newCharsLength];
        target->writeOneByte(newData + 1, 0, targetCharsLength);
        newData[0] = character;
        return new OneByteBufferPiece(newData, newCharsLength, newLineStarts, newLineStartsLength);
    }
    else
    {
        uint16_t *newData = new uint16_t[newCharsLength];
        target->write(newData + 1, 0, targetCharsLength);
        newData[0] = character;
        return new TwoByteBufferPiece(newData, newCharsLength, newLineStarts, newLineStartsLength);
    }
}

BufferPiece *BufferPiece::join2(const BufferPiece *first, const BufferPiece *second)
{
    const size_t firstCharsLength = first->length();
    const size_t secondCharsLength = second->length();

    const size_t firstLineStartsLength = first->newLineCount();
    const size_t secondLineStartsLength = second->newLineCount();

    const LINE_START_T *firstLineStarts = first->lineStarts();
    const LINE_START_T *secondLineStarts = second->lineStarts();

    const size_t newLineStartsLength = firstLineStartsLength + secondLineStartsLength;
    LINE_START_T *newLineStarts = new LINE_START_T[newLineStartsLength];
    memcpy(newLineStarts, firstLineStarts, sizeof(*newLineStarts) * firstLineStartsLength);
    for (size_t i = 0; i < secondLineStartsLength; i++)
    {
        newLineStarts[i + firstLineStartsLength] = secondLineStarts[i] + firstCharsLength;
    }

    const size_t newCharsLength = firstCharsLength + secondCharsLength;
    if (first->isOneByte() && second->isOneByte())
    {
        uint8_t *newData = new uint8_t[newCharsLength];
        first->writeOneByte(newData, 0, firstCharsLength);
        second->writeOneByte(newData + firstCharsLength, 0, secondCharsLength);
        return new OneByteBufferPiece(newData, newCharsLength, newLineStarts, newLineStartsLength);
    }
    else
    {
        uint16_t *newData = new uint16_t[newCharsLength];
        first->write(newData, 0, firstCharsLength);
        second->write(newData + firstCharsLength, 0, secondCharsLength);
        return new TwoByteBufferPiece(newData, newCharsLength, newLineStarts, newLineStartsLength);
    }
}

BufferString *recordString(BufferString *str, size_t index, vector<BufferString *> &toDelete)
{
    toDelete[index] = str;
    return str;
}

void BufferPiece::replaceOffsetLen(const BufferPiece *target, vector<LeafOffsetLenEdit2> &edits, size_t idealLeafLength, size_t maxLeafLength, vector<BufferPiece *> *result)
{
    const size_t editsSize = edits.size();
    assert(editsSize > 0);

    const size_t originalCharsLength = target->length();
    if (editsSize == 1 && edits[0].text->length() == 0 && edits[0].start == 0 && edits[0].length == originalCharsLength)
    {
        // special case => deleting everything
        return;
    }

    vector<BufferString *> toDelete(editsSize + 1);

    vector<const BufferString *> pieces(2 * editsSize + 1);
    size_t originalFromIndex = 0;
    size_t piecesTextLength = 0;
    bool resultIsOneByte = target->isOneByte();
    for (size_t i = 0; i < editsSize; i++)
    {
        LeafOffsetLenEdit2 &edit = edits[i];

        resultIsOneByte = resultIsOneByte && edit.text->containsOnlyOneByte();
        // printf("~~~~leaf edit: %lu,%lu -> [%lu ~ ", edit.start, edit.length, edit.text->length());
        // edit.text->print();
        // printf("]\n");

        // maintain the chars that survive to the left of this edit
        BufferString *originalText = recordString(BufferString::substr(target, originalFromIndex, edit.start - originalFromIndex), i, toDelete);
        pieces[2 * i] = originalText;
        piecesTextLength += originalText->length();

        originalFromIndex = edit.start + edit.length;
        pieces[2 * i + 1] = edit.text;
        piecesTextLength += edit.text->length();
    }

    // maintain the chars that survive to the right of the last edit
    BufferString *text = recordString(BufferString::substr(target, originalFromIndex, originalCharsLength - originalFromIndex), editsSize, toDelete);
    pieces[2 * editsSize] = text;
    piecesTextLength += text->length();

    size_t targetDataLength = piecesTextLength > maxLeafLength ? idealLeafLength : piecesTextLength;
    size_t targetDataOffset = 0;

    uint8_t *oneByteData = NULL;
    uint16_t *twoByteData = NULL;
    if (resultIsOneByte)
    {
        oneByteData = new uint8_t[targetDataLength];
    }
    else
    {
        twoByteData = new uint16_t[targetDataLength];
    }

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
                if (resultIsOneByte)
                {
                    result->push_back(new OneByteBufferPiece(oneByteData, targetDataLength));
                }
                else
                {
                    result->push_back(new TwoByteBufferPiece(twoByteData, targetDataLength));
                }

                targetDataLength = piecesTextLength > maxLeafLength ? idealLeafLength : piecesTextLength;
                targetDataOffset = 0;
                if (resultIsOneByte)
                {
                    oneByteData = new uint8_t[targetDataLength];
                }
                else
                {
                    twoByteData = new uint16_t[targetDataLength];
                }
            }
            size_t writingCnt = min(pieceLength - pieceOffset, targetDataLength - targetDataOffset);
            if (resultIsOneByte)
            {
                pieceText->writeOneByte(oneByteData + targetDataOffset, pieceOffset, writingCnt);
            }
            else
            {
                pieceText->write(twoByteData + targetDataOffset, pieceOffset, writingCnt);
            }

            pieceOffset += writingCnt;
            targetDataOffset += writingCnt;
            piecesTextLength -= writingCnt;

            // check that the buffer piece does not end in a \r or high surrogate
            if (targetDataOffset == targetDataLength && piecesTextLength > 0)
            {
                uint16_t lastChar;
                if (resultIsOneByte)
                {
                    lastChar = oneByteData[targetDataLength - 1];
                }
                else
                {
                    lastChar = twoByteData[targetDataLength - 1];
                }
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

    if (resultIsOneByte)
    {
        result->push_back(new OneByteBufferPiece(oneByteData, targetDataLength));
    }
    else
    {
        result->push_back(new TwoByteBufferPiece(twoByteData, targetDataLength));
    }

    for (size_t i = 0, len = toDelete.size(); i < len; i++)
    {
        delete toDelete[i];
    }
}

template <typename T>
void createLineStarts(const T *data, size_t length, vector<LINE_START_T> &lineStarts)
{
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
}

template <typename T>
void doAssertInvariants(const T *chars, size_t charsLength, const LINE_START_T *lineStarts, size_t lineStartsLength)
{
    assert(chars != NULL);
    assert(lineStarts != NULL);

    for (size_t i = 0; i < lineStartsLength; i++)
    {
        LINE_START_T lineStart = lineStarts[i];

        assert(lineStart > 0 && lineStart <= charsLength);

        if (i > 0)
        {
            LINE_START_T prevLineStart = lineStarts[i - 1];
            assert(lineStart > prevLineStart);
        }

        uint16_t charBefore = chars[lineStart - 1];
        assert(charBefore == '\n' || charBefore == '\r');

        if (charBefore == '\r' && lineStart < charsLength)
        {
            uint16_t charAfter = chars[lineStart];
            assert(charAfter != '\n');
        }
    }
}

// ---- OneByteBufferPiece

OneByteBufferPiece::OneByteBufferPiece(uint8_t *data, size_t length)
{
    assert(data != NULL);
    chars_ = data;
    charsLength_ = length;

    vector<LINE_START_T> lineStarts;
    createLineStarts(data, length, lineStarts);
    lineStarts_.assign(lineStarts);
}

OneByteBufferPiece::OneByteBufferPiece(uint8_t *data, size_t dataLength, LINE_START_T *lineStarts, size_t lineStartsLength)
{
    assert(data != NULL && lineStarts != NULL);
    chars_ = data;
    charsLength_ = dataLength;
    lineStarts_.assign(lineStarts, lineStartsLength);
}

OneByteBufferPiece::~OneByteBufferPiece()
{
    delete[] chars_;
}

void OneByteBufferPiece::assertInvariants() const
{
    doAssertInvariants(chars_, charsLength_, lineStarts_.data(), lineStarts_.length());
}

void OneByteBufferPiece::write(uint16_t *buffer, size_t start, size_t length) const
{
    assert(start + length <= charsLength_);
    for (size_t i = 0; i < length; i++)
    {
        buffer[i] = chars_[start + i];
    }
}

void OneByteBufferPiece::writeOneByte(uint8_t *buffer, size_t start, size_t length) const
{
    assert(start + length <= charsLength_);
    memcpy(buffer, chars_ + start, sizeof(*buffer) * length);
}

bool OneByteBufferPiece::containsOnlyOneByte() const {
    return true;
}

// ---- TwoByteBufferPiece

TwoByteBufferPiece::TwoByteBufferPiece(uint16_t *data, size_t length)
{
    assert(data != NULL);
    chars_ = data;
    charsLength_ = length;

    vector<LINE_START_T> lineStarts;
    createLineStarts(data, length, lineStarts);
    lineStarts_.assign(lineStarts);
}

TwoByteBufferPiece::TwoByteBufferPiece(uint16_t *data, size_t dataLength, LINE_START_T *lineStarts, size_t lineStartsLength)
{
    assert(data != NULL && lineStarts != NULL);
    chars_ = data;
    charsLength_ = dataLength;
    lineStarts_.assign(lineStarts, lineStartsLength);
}

TwoByteBufferPiece::~TwoByteBufferPiece()
{
    delete[] chars_;
}

void TwoByteBufferPiece::assertInvariants() const
{
    doAssertInvariants(chars_, charsLength_, lineStarts_.data(), lineStarts_.length());
}

void TwoByteBufferPiece::write(uint16_t *buffer, size_t start, size_t length) const
{
    assert(start + length <= charsLength_);
    memcpy(buffer, chars_ + start, sizeof(*buffer) * length);
}

void TwoByteBufferPiece::writeOneByte(uint8_t *buffer, size_t start, size_t length) const
{
    assert(start + length <= charsLength_);
    for (size_t i = 0; i < length; i++)
    {
        buffer[i] = chars_[start + i];
    }
}

bool TwoByteBufferPiece::containsOnlyOneByte() const
{
    for (size_t i = 0; i < charsLength_; i++)
    {
        if (chars_[i] >= 256)
        {
            return false;
        }
    }
    return true;
}

struct timespec time_diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
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
