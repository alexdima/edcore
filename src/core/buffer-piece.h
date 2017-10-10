/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_BUFFER_PIECE_H_
#define EDCORE_BUFFER_PIECE_H_

#include <memory>
#include <vector>
#include <cstring>
#include <time.h>
#include <assert.h>

#include "array.h"
#include "buffer-string.h"

using namespace std;

#define LINE_START_T uint32_t

namespace edcore
{

struct LeafOffsetLenEdit2
{
    size_t start;
    size_t length;
    const BufferString *text;
};
typedef struct LeafOffsetLenEdit2 LeafOffsetLenEdit2;


class BufferPiece
{
  public:
    BufferPiece(uint16_t *data, size_t len);
    ~BufferPiece();

    size_t length() const { return chars_.length(); }
    uint16_t charAt(size_t index) const { return chars_[index]; }

    size_t newLineCount() const { return lineStarts_.length(); }
    LINE_START_T lineStartFor(size_t relativeLineIndex) const { return lineStarts_[relativeLineIndex]; }

    size_t memUsage() const;

    uint16_t deleteLastChar();
    void insertFirstChar(uint16_t character);
    void join(const BufferPiece *other);
    void replaceOffsetLen(vector<LeafOffsetLenEdit2> &edits, size_t idealLeafLength, size_t maxLeafLength, vector<BufferPiece*>* result) const;

    void assertInvariants();

    void write(uint16_t *buffer, size_t start, size_t length) const {
        assert(start + length <= chars_.length());
        const uint16_t *src = chars_.data();
        memcpy(buffer, src + start, sizeof(*buffer) * length);
    }

  private:
    BufferPiece(uint16_t *data, size_t dataLength, LINE_START_T *lineStarts, size_t lineStartsLength);

    MyArray<uint16_t> chars_;
    MyArray<LINE_START_T> lineStarts_;

    void _rebuildLineStarts();
};

struct timespec time_diff(struct timespec start, struct timespec end);
void print_diff(const char *pre, struct timespec start);
}

#endif
