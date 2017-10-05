/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <iostream>
#include "ed-buffer.h"

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
    NODE_SET_PROTOTYPE_METHOD(tpl, "AssertInvariants", AssertInvariants);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(v8::String::NewFromUtf8(isolate, "EdBuffer"),
                 tpl->GetFunction());
}
