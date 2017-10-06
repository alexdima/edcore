/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <iostream>
#include "ed-buffer.h"
#include <algorithm>

using namespace std;

class MyString : public v8::String::ExternalStringResource
{
  public:
    MyString(const uint16_t *data, size_t length) : data_(data), length_(length) {}
    ~MyString() { delete[] data_; }
    virtual const uint16_t *data() const { return data_; }
    virtual size_t length() const { return length_; }

  private:
    const uint16_t *data_;
    size_t length_;
};

EdBuffer::EdBuffer(EdBufferBuilder *builder)
{
    this->actual_ = builder->BuildBuffer();
}

EdBuffer::~EdBuffer()
{
    delete this->actual_;
}

void EdBuffer::GetLength(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();
    EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

    args.GetReturnValue().Set(v8::Number::New(isolate, obj->actual_->length()));
}

void EdBuffer::GetLineCount(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();
    EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

    args.GetReturnValue().Set(v8::Number::New(isolate, obj->actual_->lineCount()));
}

void EdBuffer::GetLineContent(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();
    EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

    if (!args[0]->IsNumber())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Argument must be a number")));
        return;
    }

    size_t lineNumber = args[0]->NumberValue();

    edcore::BufferCursor start, end;
    if (!obj->actual_->findLine(lineNumber, start, end))
    {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Line not found")));
        return;
    }

    size_t len = end.offset - start.offset;
    uint16_t *data = new uint16_t[len];
    obj->actual_->extractString(start, len, data);
    v8::MaybeLocal<v8::String> res = v8::String::NewExternalTwoByte(isolate, new MyString(data, len));
    args.GetReturnValue().Set(res.ToLocalChecked() /*TODO*/);
}

void EdBuffer::DeleteOneOffsetLen(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();
    EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

    if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsNumber())
    {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Expected two number arguments")));
        return;
    }

    size_t offset = args[0]->NumberValue();
    size_t len = args[1]->NumberValue();
    const size_t bufferLength = obj->actual_->length();

    if (offset > bufferLength || offset + len > bufferLength)
    {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Invalid range")));
        return;
    }

    // size_t memUsageBefore = obj->actual_->memUsage();

    obj->actual_->deleteOneOffsetLen(offset, len);

    // size_t memUsageAfter = obj->actual_->memUsage();
    // printf("mem usage: %lf KB -> %lf KB\n", ((double)memUsageBefore) / 1024, ((double)memUsageAfter) / 1024);
}

void EdBuffer::InsertOneOffsetLen(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();
    EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

    if (args.Length() != 2 || !args[0]->IsNumber() || !args[1]->IsString())
    {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Expected two arguments, first a number and second a string")));
        return;
    }

    size_t offset = args[0]->NumberValue();

    const size_t bufferLength = obj->actual_->length();
    if (offset > bufferLength)
    {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Invalid offset")));
        return;
    }

    v8::Local<v8::String> text = v8::Local<v8::String>::Cast(args[1]);
    v8::String::Value utf16Value(text);

    // size_t memUsageBefore = obj->actual_->memUsage();
    obj->actual_->insertOneOffsetLen(offset, *utf16Value, utf16Value.length());
    // size_t memUsageAfter = obj->actual_->memUsage();
    // printf("mem usage: %lf KB -> %lf KB\n", ((double)memUsageBefore) / 1024, ((double)memUsageAfter) / 1024);
}

bool compareEdits(edcore::OffsetLenEdit &a, edcore::OffsetLenEdit &b)
{
    if (a.offset == b.offset)
    {
        return (a.initialIndex < b.initialIndex);
    }
    return (a.offset < b.offset);
}

void EdBuffer::ReplaceOffsetLen(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();
    v8::Local<v8::Context> ctx = isolate->GetCurrentContext();
    EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

    if (args.Length() != 1 || !args[0]->IsArray())
    {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Expected one array argument")));
        return;
    }
    v8::Local<v8::Array> _edits = v8::Local<v8::Array>::Cast(args[0]);

    v8::Local<v8::String> offsetStr = v8::String::NewFromUtf8(isolate, "offset", v8::NewStringType::kNormal).ToLocalChecked();
    v8::Local<v8::String> lengthStr = v8::String::NewFromUtf8(isolate, "length", v8::NewStringType::kNormal).ToLocalChecked();
    v8::Local<v8::String> textStr = v8::String::NewFromUtf8(isolate, "text", v8::NewStringType::kNormal).ToLocalChecked();

    const size_t maxPosition = obj->actual_->length();

    size_t totalDataLength = 0;
    vector<edcore::OffsetLenEdit> edits(_edits->Length());
    for (size_t i = 0; i < _edits->Length(); i++)
    {
        v8::Local<v8::Value> _element = _edits->Get(i);
        if (!_element->IsObject())
        {
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, "Expected object in array elements")));
            return;
        }

        v8::Local<v8::Object> element = v8::Local<v8::Object>::Cast(_element);

        v8::MaybeLocal<v8::Value> maybeOffset_ = element->GetRealNamedProperty(ctx, offsetStr);
        v8::Local<v8::Value> offset_;
        if (!maybeOffset_.ToLocal(&offset_) || !offset_->IsNumber())
        {
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, "Expected .offset to be a number")));
            return;
        }

        v8::MaybeLocal<v8::Value> maybeLength_ = element->GetRealNamedProperty(ctx, lengthStr);
        v8::Local<v8::Value> length_;
        if (!maybeLength_.ToLocal(&length_) || !length_->IsNumber())
        {
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, "Expected .length to be a number")));
            return;
        }

        v8::MaybeLocal<v8::Value> maybeText_ = element->GetRealNamedProperty(ctx, textStr);
        v8::Local<v8::Value> text_;
        if (!maybeText_.ToLocal(&text_) || !text_->IsString())
        {
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, "Expected .text to be a string")));
            return;
        }

        size_t offset = offset_->NumberValue();
        size_t length = length_->NumberValue();

        // Validate that edit is within bounds
        if (offset > maxPosition)
        {
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, "Invalid position")));
            return;
        }
        if (offset + length > maxPosition)
        {
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, "Invalid length")));
            return;
        }

        edits[i].initialIndex = i;
        edits[i].offset = offset;
        edits[i].length = length;
        v8::Local<v8::String> text = v8::Local<v8::String>::Cast(text_);
        // v8::String::Value utf16Value(text);
        edits[i].data = NULL;//*utf16Value;
        edits[i].dataLength = text->Length();//utf16Value->length();

        totalDataLength += text->Length();
    }

    // Sort edits
    std::sort(edits.begin(), edits.end(), compareEdits);

    // Check that there are no overlapping edits
    for (size_t i = 1; i < edits.size(); i++)
    {
        edcore::OffsetLenEdit &prev = edits[i - 1];
        edcore::OffsetLenEdit &curr = edits[i];
        if (prev.offset + prev.length > curr.offset)
        {
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8(isolate, "Invalid edits: overlapping")));
            return;
        }
    }

    // Extract data
    // printf("totalDataLength: %lu\n", totalDataLength);
    uint16_t *allData = new uint16_t[totalDataLength];
    uint16_t *currData = allData;
    for (size_t i = 0; i < edits.size(); i++)
    {
        edcore::OffsetLenEdit &edit = edits[i];
        v8::Local<v8::Value> _element = _edits->Get(edit.initialIndex);
        v8::Local<v8::Object> element = v8::Local<v8::Object>::Cast(_element);
        v8::Local<v8::Value> text_ = element->GetRealNamedProperty(ctx, textStr).ToLocalChecked();
        v8::Local<v8::String> text = v8::Local<v8::String>::Cast(text_);
        v8::String::Value utf16Value(text);

        const uint16_t *utf16Data = *utf16Value;
        // for (size_t j = 0; j < text->Length(); j++)
        // {
        //     printf("  -> %lu\n", utf16Data[j]);
        // }

        memcpy(currData, utf16Data, sizeof(uint16_t) * text->Length());
        edit.data = currData;

        currData += text->Length();
        // v8::Local<v8::Value> text_;
        // if (!maybeText_.ToLocal(&text_) || !text_->IsString())
        // {
        //     isolate->ThrowException(v8::Exception::Error(
        //         v8::String::NewFromUtf8(isolate, "Expected .text to be a string")));
        //     return;
        // }
    }

    // assert(false);

    obj->actual_->replaceOffsetLen(edits);

    delete []allData;
}

void EdBuffer::AssertInvariants(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());
    obj->actual_->assertInvariants();
}

v8::Local<v8::Object> EdBuffer::Create(v8::Isolate *isolate, const v8::Local<v8::Object> builder)
{
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = {builder};
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, EdBuffer::constructor);
    v8::Local<v8::Object> result =
        cons->NewInstance(context, argc, argv).ToLocalChecked();
    return result;
}

void EdBuffer::New(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();

    if (args.IsConstructCall())
    {
        // Invoked as constructor: `new MyObject(...)`
        EdBufferBuilder *builder = ObjectWrap::Unwrap<EdBufferBuilder>(args[0]->ToObject());
        EdBuffer *obj = new EdBuffer(builder);
        obj->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
    }
    else
    {
        // Invoked as plain function `MyObject(...)`, turn into construct call.
        const int argc = 1;
        v8::Local<v8::Value> argv[argc] = {args[0]};
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
        v8::Local<v8::Object> result =
            cons->NewInstance(context, argc, argv).ToLocalChecked();
        args.GetReturnValue().Set(result);
    }
}

void EdBuffer::Init(v8::Local<v8::Object> exports)
{
    v8::Isolate *isolate = exports->GetIsolate();

    // Prepare constructor template
    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
    tpl->SetClassName(v8::String::NewFromUtf8(isolate, "EdBuffer"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "GetLength", GetLength);
    NODE_SET_PROTOTYPE_METHOD(tpl, "GetLineCount", GetLineCount);
    NODE_SET_PROTOTYPE_METHOD(tpl, "GetLineContent", GetLineContent);
    NODE_SET_PROTOTYPE_METHOD(tpl, "DeleteOneOffsetLen", DeleteOneOffsetLen);
    NODE_SET_PROTOTYPE_METHOD(tpl, "InsertOneOffsetLen", InsertOneOffsetLen);
    NODE_SET_PROTOTYPE_METHOD(tpl, "ReplaceOffsetLen", ReplaceOffsetLen);
    NODE_SET_PROTOTYPE_METHOD(tpl, "AssertInvariants", AssertInvariants);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(v8::String::NewFromUtf8(isolate, "EdBuffer"),
                 tpl->GetFunction());
}
