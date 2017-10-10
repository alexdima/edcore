/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_BUFFER_STRING_H_
#define EDCORE_BUFFER_STRING_H_

#include <memory>
#include <vector>
#include <cstring>
#include <stdint.h>
#include <stdio.h>

using namespace std;

namespace edcore
{
class BufferString
{
  public:
    virtual ~BufferString(){};
    /**
     * Returns the number of characters in this string.
     */
    virtual size_t length() const = 0;

    // 16-bit character codes.
    virtual void write(uint16_t *buffer, size_t start, size_t length) const = 0;

    // One byte characters.
    virtual void writeOneByte(uint8_t *buffer, size_t start, size_t length) const = 0;

    /**
     * Returns whether this string is known to contain only one byte data.
     * Does not read the string.
     * False negatives are possible.
     */
    virtual bool isOneByte() const = 0;

    /**
     * Returns whether this string contain only one byte data.
     * Will read the entire string in some cases.
     */
    virtual bool containsOnlyOneByte() const = 0;

    static BufferString *createFromSingle(uint16_t character);

    static BufferString *concat(const BufferString *a, const BufferString *b);

    static BufferString *substr(const BufferString *target, size_t start, size_t length);

    /**
     * A zero length string.
     */
    static BufferString *empty();

    /**
     * A string containing carriage return (\r).
     */
    static BufferString *carriageReturn();

    /**
     * A string containing line feed (\n).
     */
    static BufferString *lineFeed();

    void print() const
    {
        size_t len = this->length();
        uint16_t *target = new uint16_t[len];
        this->write(target, 0, len);

        char *chars = new char[len + 1];
        for (size_t i = 0; i < len; i++)
        {
            chars[i] = target[i];
        }
        chars[len] = 0;
        printf("<<%s>>", chars);
        delete[] target;
        delete[] chars;
    }
};

class SingleOneByteString : public BufferString
{
  public:
    SingleOneByteString(uint8_t character) { character_ = character; }
    size_t length() const { return 1; }
    void write(uint16_t *buffer, size_t start, size_t length) const { buffer[0] = character_; }
    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const { buffer[0] = character_; }
    bool isOneByte() const { return true; }
    bool containsOnlyOneByte() const { return true; }

  private:
    uint8_t character_;
};

class SingleTwoByteString : public BufferString
{
  public:
    SingleTwoByteString(uint16_t character) { character_ = character; }
    size_t length() const { return 1; }
    void write(uint16_t *buffer, size_t start, size_t length) const { buffer[0] = character_; }
    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const { buffer[0] = character_; }
    bool isOneByte() const { return false; }
    bool containsOnlyOneByte() const { return false; }

  private:
    uint16_t character_;
};

class EmptyString : public BufferString
{
  public:
    EmptyString(){};
    size_t length() const { return 0; }
    void write(uint16_t *buffer, size_t start, size_t length) const {}
    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const {}
    bool isOneByte() const { return true; }
    bool containsOnlyOneByte() const { return true; }
};

class ConcatString : public BufferString
{
  public:
    ConcatString(const BufferString *left, const BufferString *right)
    {
        left_ = left;
        right_ = right;
    }
    size_t length() const { return left_->length() + right_->length(); }
    void write(uint16_t *buffer, size_t start, size_t length) const;
    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const;
    bool isOneByte() const { return left_->isOneByte() && right_->isOneByte(); }
    bool containsOnlyOneByte() const { return left_->containsOnlyOneByte() && right_->containsOnlyOneByte(); }

  private:
    const BufferString *left_;
    const BufferString *right_;
};

class SubString : public BufferString
{
  public:
    SubString(const BufferString *target, size_t start, size_t length)
    {
        target_ = target;
        start_ = start;
        length_ = length;
    }
    size_t length() const { return length_; }
    void write(uint16_t *buffer, size_t start, size_t length) const;
    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const;
    bool isOneByte() const { return target_->isOneByte(); /* TODO! a substring could become one byte */ }
    bool containsOnlyOneByte() const { return target_->containsOnlyOneByte(); /* TODO! a substring could become one byte */ }

  private:
    const BufferString *target_;
    size_t start_;
    size_t length_;
};
}

#endif
