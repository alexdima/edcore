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

class BufferPiece : public BufferString
{
  public:
    virtual ~BufferPiece(){};

    size_t newLineCount() const { return lineStarts_.length(); }
    LINE_START_T lineStartFor(size_t relativeLineIndex) const { return lineStarts_[relativeLineIndex]; }
    const LINE_START_T *lineStarts() const { return lineStarts_.data(); }

    virtual size_t length() const = 0;
    virtual uint16_t charAt(size_t index) const = 0;
    virtual size_t memUsage() const = 0;

    virtual void assertInvariants() const = 0;
    // virtual void write(uint16_t *buffer, size_t start, size_t length) const = 0;

    static BufferPiece *createFromString(const BufferString *str);
    static BufferPiece *deleteLastChar2(const BufferPiece *target);
    static BufferPiece *insertFirstChar2(const BufferPiece *target, uint16_t character);
    static BufferPiece *join2(const BufferPiece *first, const BufferPiece *second);
    static void replaceOffsetLen(const BufferPiece *target, vector<LeafOffsetLenEdit2> &edits, size_t idealLeafLength, size_t maxLeafLength, vector<BufferPiece *> *result);

  protected:
    MyArray<LINE_START_T> lineStarts_;
};

class OneByteBufferPiece : public BufferPiece
{
  public:
    OneByteBufferPiece(uint8_t *data, size_t len);
    OneByteBufferPiece(uint8_t *data, size_t dataLength, LINE_START_T *lineStarts, size_t lineStartsLength);
    ~OneByteBufferPiece();

    void assertInvariants() const;

    size_t memUsage() const { return (sizeof(OneByteBufferPiece) + (charsLength_ * sizeof(*chars_)) + lineStarts_.memUsage()); }
    size_t length() const { return charsLength_; }
    uint16_t charAt(size_t index) const { return chars_[index]; }
    bool isOneByte() const { return true; }

    void write(uint16_t *buffer, size_t start, size_t length) const;
    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const;
    bool containsOnlyOneByte() const;

  private:
    uint8_t *chars_;
    size_t charsLength_;
};

class TwoByteBufferPiece : public BufferPiece
{
  public:
    TwoByteBufferPiece(uint16_t *data, size_t len);
    TwoByteBufferPiece(uint16_t *data, size_t dataLength, LINE_START_T *lineStarts, size_t lineStartsLength);
    ~TwoByteBufferPiece();

    void assertInvariants() const;

    size_t memUsage() const { return (sizeof(TwoByteBufferPiece) + (charsLength_ * sizeof(*chars_)) + lineStarts_.memUsage()); }
    size_t length() const { return charsLength_; }
    uint16_t charAt(size_t index) const { return chars_[index]; }
    bool isOneByte() const { return false; }

    void write(uint16_t *buffer, size_t start, size_t length) const;
    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const;
    bool containsOnlyOneByte() const;

  private:
    uint16_t *chars_;
    size_t charsLength_;
};

struct timespec time_diff(struct timespec start, struct timespec end);
void print_diff(const char *pre, struct timespec start);
}

#endif
