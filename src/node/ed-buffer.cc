/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include <iostream>
#include "ed-buffer.h"

using namespace std;

EdBuffer::EdBuffer(EdBufferBuilder *builder)
{
    this->_actual = builder->Build();
}

EdBuffer::~EdBuffer()
{
    delete this->_actual;
}

void EdBuffer::GetLineCount(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();
    EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

    args.GetReturnValue().Set(v8::Number::New(isolate, obj->_actual->getLineCount()));
}

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

    if (!obj->_actual->findLine(lineNumber, start, end))
    {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, "Line not found")));
        return;
    }

    // cout << "FOUND LINE START FOR " << lineNumber << " AT (" << start.offset << "," << start.nodeStartOffset << ") up to (" << end.offset << "," << end.nodeStartOffset << ") i.e. " << (end.offset - start.offset) << endl;
    // assert(start.offset >= start.nodeStartOffset);
    // assert(start.offset <= start.nodeStartOffset + start.node->getLen());
    // assert(end.offset >= end.nodeStartOffset);
    // assert(end.offset <= end.nodeStartOffset + end.node->getLen());
    // assert(end.offset >= start.offset);
    size_t len = end.offset - start.offset;
    uint16_t *alt = new uint16_t[len];
    obj->_actual->extractString(start, len, alt);

    // assert(str->getLen() == (end.offset - start.offset));
    // for (int i = 0; i < str->getLen(); i++)
    // {
    //     assert(tmp[i] == alt[i]);
    // }

    // v8::Local<v8::String> = 

    v8::MaybeLocal<v8::String> res = v8::String::NewExternalTwoByte(isolate, new MyString(alt, len));
    // delete[] alt;
    // }

    // delete[] tmp;
    args.GetReturnValue().Set(res.ToLocalChecked() /*TODO*/);
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
    NODE_SET_PROTOTYPE_METHOD(tpl, "GetLineCount", GetLineCount);
    NODE_SET_PROTOTYPE_METHOD(tpl, "GetLineContent", GetLineContent);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(v8::String::NewFromUtf8(isolate, "EdBuffer"),
                 tpl->GetFunction());
}
