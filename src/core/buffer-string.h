/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_BUFFER_STRING_H_
#define EDCORE_BUFFER_STRING_H_

#include <memory>
#include <vector>
#include <cstring>

using namespace std;

namespace edcore
{
class BufferString
{
public:
    /**
     * Returns the number of characters in this string.
     */
    virtual size_t length() const;

    // 16-bit character codes.
    virtual void write(uint16_t *buffer, size_t start, size_t length) const;

    // One byte characters.
    virtual void writeOneByte(uint8_t *buffer, size_t start, size_t length) const;

    /**
     * Returns whether this string is known to contain only one byte data.
     * Does not read the string.
     * False negatives are possible.
     */
    virtual bool isOneByte() const;

    /**
     * Returns whether this string contain only one byte data.
     * Will read the entire string in some cases.
     */
    virtual bool containsOnlyOneByte() const;
};
}

#endif
