/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_BUFFER_PIECE_H_
#define EDCORE_BUFFER_PIECE_H_

#include <memory>

using namespace std;

namespace edcore
{

class BufferPiece
{
  public:
    BufferPiece(uint16_t *data, size_t len);
    ~BufferPiece();

    const uint16_t *data() const { return this->data_; }
    size_t length() const { return this->length_; }
    const size_t *lineStarts() const { return this->lineStarts_; }
    size_t newLineCount() const { return this->lineStartsCount_; }

    void deleteOneOffsetLen(size_t offset, size_t len);

    void print(ostream &os) const;

  private:
    uint16_t *data_;
    size_t length_;
    size_t dataCapacity_;

    size_t *lineStarts_;
    size_t lineStartsCount_;
    size_t lineStartsCapacity_;

    void _init(uint16_t *data, size_t len, size_t *lineStarts, size_t lineStartsCount);
};
}

#endif
