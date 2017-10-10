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
    virtual ~BufferPiece() {};

    size_t newLineCount() const { return lineStarts_.length(); }
    LINE_START_T lineStartFor(size_t relativeLineIndex) const { return lineStarts_[relativeLineIndex]; }
    const LINE_START_T* lineStarts() const { return lineStarts_.data(); }

    virtual size_t length() const = 0;
    virtual uint16_t charAt(size_t index) const = 0;
    virtual size_t memUsage() const = 0;

    virtual void assertInvariants() const = 0;
    virtual void write(uint16_t *buffer, size_t start, size_t length) const = 0;

    static void replaceOffsetLen(const BufferPiece *target, vector<LeafOffsetLenEdit2> &edits, size_t idealLeafLength, size_t maxLeafLength, vector<BufferPiece*>* result);
    static BufferPiece *deleteLastChar2(const BufferPiece *target);
    static BufferPiece *insertFirstChar2(const BufferPiece *target, uint16_t character);
    static BufferPiece * join2(const BufferPiece *first, const BufferPiece *second);

  protected:
    MyArray<LINE_START_T> lineStarts_;
};

// class OneByteBufferPiece : public BufferPiece {

// private:
//     uint8_t* chars_;
// };

class TwoBytesBufferPiece: public BufferPiece {
public:
    TwoBytesBufferPiece(uint16_t *data, size_t len);
    TwoBytesBufferPiece(uint16_t *data, size_t dataLength, LINE_START_T *lineStarts, size_t lineStartsLength);
    ~TwoBytesBufferPiece();

    size_t memUsage() const { return (sizeof(TwoBytesBufferPiece) + chars_.memUsage() + lineStarts_.memUsage()); }
    size_t length() const { return chars_.length(); }
    uint16_t charAt(size_t index) const { return chars_[index]; }

    void write(uint16_t *buffer, size_t start, size_t length) const {
        assert(start + length <= chars_.length());
        const uint16_t *src = chars_.data();
        memcpy(buffer, src + start, sizeof(*buffer) * length);
    }

    void assertInvariants() const;

private:

    MyArray<uint16_t> chars_;
};

struct timespec time_diff(struct timespec start, struct timespec end);
void print_diff(const char *pre, struct timespec start);
}

#endif
