/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <iostream>
#include <assert.h>
#include <cstring>

#include "buffer-string.h"

namespace edcore
{

BufferString* BufferString::empty() {
    static EmptyString value;
    return &value;
}

BufferString* BufferString::carriageReturn() {
    static SingleByteString value('\r');
    return &value;
}

BufferString* BufferString::lineFeed() {
    static SingleByteString value('\n');
    return &value;
}

BufferString* BufferString::concat(const BufferString*a, const BufferString*b) {
    return new ConcatString(a,b);
}

BufferString* BufferString::substr(const BufferString*target, size_t start, size_t length) {
    return new SubString(target, start, length);
}


void ConcatString::write(uint16_t *buffer, size_t start, size_t length) const {
    assert(start + length <= this->length());

    const size_t leftLength = left_->length();

    size_t leftCnt = 0;
    if (start < leftLength)
    {
        leftCnt = min(leftLength - start, length);
        left_->write(buffer, start, leftCnt);
    }

    if (leftCnt < length)
    {
        right_->write(buffer + leftCnt, (start >= leftLength ? start - leftLength : 0), length - leftCnt);
    }
}

void ConcatString::writeOneByte(uint8_t *buffer, size_t start, size_t length) const {
    assert(false); // broken
    assert(start + length <= this->length());

    const size_t leftLength = left_->length();

    size_t leftCnt = 0;
    if (start < leftLength)
    {
        leftCnt = leftLength - start;
        left_->writeOneByte(buffer, start, leftCnt);
    }

    if (leftCnt < length)
    {
        right_->writeOneByte(buffer + leftCnt, start - leftCnt, length - leftCnt);
    }
}

void SubString::write(uint16_t *buffer, size_t start, size_t length) const {
    assert(start + length <= this->length());
    target_->write(buffer, start + start_, length);
}

void SubString::writeOneByte(uint8_t *buffer, size_t start, size_t length) const {
    assert(start + length <= this->length());
    target_->writeOneByte(buffer, start + start_, length);
}

}
