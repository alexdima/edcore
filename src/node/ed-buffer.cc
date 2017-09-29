/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "ed-buffer.h"

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

void EdBuffer::GetLineContent(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();
    EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

    size_t lineNumber = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();



    shared_ptr<edcore::String> str = obj->_actual->getLineContent(lineNumber);
    uint16_t *tmp = new uint16_t[str->getLen()];
    str->writeTo(tmp);
    v8::MaybeLocal<v8::String> res = v8::String::NewFromTwoByte(isolate, tmp, v8::NewStringType::kNormal, str->getLen());
    delete[] tmp;
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
