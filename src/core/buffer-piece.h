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

#include "array.h"
#include "buffer-string.h"

using namespace std;

#define LINE_START_T uint32_t

namespace edcore
{

struct LeafOffsetLenEdit
{
    size_t start;
    size_t length;
    const uint16_t *data;
    size_t dataLength;

    size_t resultStart;
};
typedef struct LeafOffsetLenEdit LeafOffsetLenEdit;

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

    const uint16_t *data() const { return chars_.data(); }
    size_t length() const { return chars_.length(); }
    size_t capacity() const { return chars_.capacity(); }
    const LINE_START_T *lineStarts() const { return lineStarts_.data(); }
    size_t newLineCount() const { return lineStarts_.length(); }
    size_t memUsage() const;

    void deleteOneOffsetLen(size_t offset, size_t len);
    void insertOneOffsetLen(size_t offset, const uint16_t *data, size_t len);
    uint16_t deleteLastChar();
    void insertFirstChar(uint16_t character);
    void join(const BufferPiece *other);
    void split(size_t idealLeafLength, vector<BufferPiece*> &dest) const;
    void replaceOffsetLen(vector<LeafOffsetLenEdit> &edits);

    void replaceOffsetLen(vector<LeafOffsetLenEdit2> &edits, size_t idealLeafLength, size_t maxLeafLength, vector<BufferPiece*>* result) const;

    void assertInvariants();

  private:

    BufferPiece(uint16_t *data, size_t dataLength, LINE_START_T *lineStarts, size_t lineStartsLength);


    MyArray<uint16_t> chars_;
    MyArray<LINE_START_T> lineStarts_;
    bool hasLonelyCR_;

    void _rebuildLineStarts();
    bool _tryApplyEditsNoAllocate(vector<LeafOffsetLenEdit> &edits, size_t newLength);
    void _applyEditsAllocate(vector<LeafOffsetLenEdit> &edits, size_t newLength);
};

struct timespec time_diff(struct timespec start, struct timespec end);
void print_diff(const char *pre, struct timespec start);
}

#endif
