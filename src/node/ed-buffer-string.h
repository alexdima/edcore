/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef SRC_ED_BUFFER_STRING_H_
#define SRC_ED_BUFFER_STRING_H_

#include <node.h>
#include <node_object_wrap.h>

class v8StringAsBufferString : public edcore::BufferString
{
  private:
    v8::Local<v8::String> source_;

  public:
    v8StringAsBufferString(v8::Local<v8::String> &source) : source_(source)
    {
    }

    size_t length() const
    {
        return source_->Length();
    }

    void write(uint16_t *buffer, size_t start, size_t length) const
    {
        source_->Write(buffer, start, length, v8::String::WriteOptions::NO_NULL_TERMINATION | v8::String::WriteOptions::HINT_MANY_WRITES_EXPECTED);
    }

    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const
    {
        source_->WriteOneByte(buffer, start, length, v8::String::WriteOptions::NO_NULL_TERMINATION | v8::String::WriteOptions::HINT_MANY_WRITES_EXPECTED);
    }

    bool isOneByte() const
    {
        return source_->IsOneByte();
    }

    bool containsOnlyOneByte() const
    {
        return source_->ContainsOnlyOneByte();
    }
};

#endif
