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

TwoBytesBufferPiece::TwoBytesBufferPiece(uint16_t *___data, size_t len)
{
    assert(___data != NULL);
    chars_.assign(___data, len);


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

TwoBytesBufferPiece::TwoBytesBufferPiece(uint16_t *data, size_t dataLength, LINE_START_T *lineStarts, size_t lineStartsLength)
{
    chars_.assign(data, dataLength);
    lineStarts_.assign(lineStarts, lineStartsLength);
}

TwoBytesBufferPiece::~TwoBytesBufferPiece()
{
}

BufferPiece *TwoBytesBufferPiece::deleteLastChar2() const
{
    const size_t charsLength = chars_.length();
    const size_t lineStartsLength = lineStarts_.length();

    size_t newLineStartsLength;
    if (lineStartsLength > 0 && lineStarts_[lineStartsLength - 1] == charsLength)
    {
        newLineStartsLength = lineStartsLength - 1;
    }
    else
    {
        newLineStartsLength = lineStartsLength;
    }

    const size_t newCharsLength = charsLength - 1;
    uint16_t *data = new uint16_t[newCharsLength];
    this->write(data, 0, newCharsLength);

    LINE_START_T *lineStarts = new LINE_START_T[newLineStartsLength];
    memcpy(lineStarts, lineStarts_.data(), sizeof(*lineStarts) * newLineStartsLength);

    return new TwoBytesBufferPiece(data, newCharsLength, lineStarts, newLineStartsLength);
}

BufferPiece *TwoBytesBufferPiece::insertFirstChar2(uint16_t character) const
{
    const size_t charsLength = chars_.length();
    const size_t lineStartsLength = lineStarts_.length();
    const bool insertLineStart = (
        (character == '\r' && (lineStartsLength == 0 || lineStarts_[0] != 1 || chars_[0] != '\n'))
        || (character == '\n')
    );

    const size_t newCharsLength = charsLength + 1;
    uint16_t *data = new uint16_t[newCharsLength];
    this->write(data + 1, 0, charsLength);
    data[0] = character;

    const size_t newLineStartsLength = (insertLineStart ? lineStartsLength + 1 : lineStartsLength);
    LINE_START_T *lineStarts = new LINE_START_T[newLineStartsLength];

    if (insertLineStart) {
        lineStarts[0] = 1;
        for (size_t i = 0; i < lineStartsLength; i++)
        {
            lineStarts[i + 1] = lineStarts_[i] + 1;
        }
    } else {
        for (size_t i = 0; i < lineStartsLength; i++)
        {
            lineStarts[i] = lineStarts_[i] + 1;
        }
    }

    return new TwoBytesBufferPiece(data, newCharsLength, lineStarts, newLineStartsLength);
}

BufferPiece * TwoBytesBufferPiece::join2(const BufferPiece *other) const
{
    const size_t charsLength = chars_.length();
    const size_t otherCharsLength = other->length();
    const size_t newCharsLength = charsLength + otherCharsLength;

    uint16_t *data = new uint16_t[newCharsLength];
    this->write(data, 0, charsLength);
    other->write(data + charsLength, 0, otherCharsLength);

    const size_t lineStartsLength = lineStarts_.length();
    const size_t otherLineStartsLength = other->newLineCount();
    const size_t newLineStartsLength = lineStartsLength + otherLineStartsLength;

    LINE_START_T *lineStarts = new LINE_START_T[newLineStartsLength];
    memcpy(lineStarts, lineStarts_.data(), sizeof(*lineStarts) * lineStartsLength);
    const LINE_START_T *otherLineStarts = other->lineStarts();
    for (size_t i = 0; i < otherLineStartsLength; i++)
    {
        lineStarts[i + lineStartsLength] = otherLineStarts[i] + charsLength;
    }

    return new TwoBytesBufferPiece(data, newCharsLength, lineStarts, newLineStartsLength);
}

class BufferPieceString : public BufferString
{
public:
    BufferPieceString(const TwoBytesBufferPiece *target) { target_ = target; }
    size_t length() const { return target_->length(); }
    void write(uint16_t *buffer, size_t start, size_t length) const {
        target_->write(buffer, start, length);
    }
    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const { assert(false); /* TODO! */ }
    bool isOneByte() const { return false; /* TODO! */ }
    bool containsOnlyOneByte() const { return false; /* TODO! */ }
private:
    const TwoBytesBufferPiece *target_;
};

BufferString * recordString(BufferString *str, size_t index, vector<BufferString*> &toDelete)
{
    toDelete[index] = str;
    return str;
}

void TwoBytesBufferPiece::replaceOffsetLen(vector<LeafOffsetLenEdit2> &edits, size_t idealLeafLength, size_t maxLeafLength, vector<BufferPiece*>* result) const
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
                result->push_back(new TwoBytesBufferPiece(targetData, targetDataLength));

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

    result->push_back(new TwoBytesBufferPiece(targetData, targetDataLength));

    for (size_t i = 0, len = toDelete.size(); i < len; i++)
    {
        delete toDelete[i];
    }
}

void TwoBytesBufferPiece::assertInvariants() const
{
    const size_t charsLength = chars_.length();
    const size_t lineStartsLength = lineStarts_.length();

    assert(chars_.data() != NULL);
    assert(lineStarts_.data() != NULL);

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
