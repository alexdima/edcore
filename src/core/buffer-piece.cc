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

    LINE_START_T *lineStarts = new LINE_START_T[lineStartsCount];

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
    _init(data, len, lineStarts, lineStartsCount);
}

void BufferPiece::_init(uint16_t *data, size_t len, LINE_START_T *lineStarts, size_t lineStartsCount)
{
    chars_.init(data, len);
    lineStarts_.init(lineStarts, lineStartsCount);
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

void BufferPiece::insertOneOffsetLen(size_t offset, const uint16_t *data, size_t len)
{
    const size_t charsLength = chars_.length();
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

}
