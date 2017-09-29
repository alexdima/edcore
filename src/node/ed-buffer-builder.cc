
#include "ed-buffer-builder.h"
#include "ed-buffer.h"

EdBufferBuilder::EdBufferBuilder()
{
    this->_actual = new edcore::BufferBuilder();
}

EdBufferBuilder::~EdBufferBuilder()
{
    if (this->_actual != NULL)
    {
        delete this->_actual;
        this->_actual = NULL;
    }
}

void EdBufferBuilder::New(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();

    if (args.IsConstructCall())
    {
        // Invoked as constructor: `new MyObject(...)`
        EdBufferBuilder *obj = new EdBufferBuilder();
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

void EdBufferBuilder::AcceptChunk(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();
    EdBufferBuilder *obj = ObjectWrap::Unwrap<EdBufferBuilder>(args.Holder());

    v8::Local<v8::String> chunk = v8::Local<v8::String>::Cast(args[0]);
    if (!chunk->IsString())
    {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, "Argument must be a string")));
        return;
    }

    v8::String::Value utf16Value(chunk);

    obj->_actual->AcceptChunk(*utf16Value, utf16Value.length());
}

void EdBufferBuilder::Finish(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    // v8::Isolate *isolate = args.GetIsolate();
    EdBufferBuilder *obj = ObjectWrap::Unwrap<EdBufferBuilder>(args.Holder());

    obj->_actual->Finish();
}

void EdBufferBuilder::Build(const v8::FunctionCallbackInfo<v8::Value> &args)
{
    v8::Isolate *isolate = args.GetIsolate();
    v8::Local<v8::Object> result =
        EdBuffer::Create(args.GetIsolate(), args.Holder());
    // const int argc = 1;
    // v8::Local<v8::Value> argv[argc] = {args.Holder()};
    // v8::Local<v8::Context> context = isolate->GetCurrentContext();

    // v8::FunctionCallbackInfo info = v8::FunctionCallbackInfo::

    //     EdBuffer::New()
    //     // EdBufferBuilder *obj = ObjectWrap::Unwrap<EdBufferBuilder>(args.Holder());

    //     v8::Local<v8::Function>
    //         cons = v8::Local<v8::Function>::New(isolate, EdBuffer::constructor);
    // v8::Local<v8::Object> result =
    //     cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(result);
}

// Build(): EdBuffer;

// static void GetLineContent(const v8::FunctionCallbackInfo<v8::Value> &args)
// {
//     v8::Isolate *isolate = args.GetIsolate();
//     EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

//     size_t lineNumber = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();

//     shared_ptr<String> str = obj->_actual->getLineContent(lineNumber);
//     char *tmp = new char[str->getLen()];
//     str->writeTo(tmp);
//     v8::MaybeLocal<v8::String> res = v8::String::NewFromUtf8(isolate, tmp, v8::NewStringType::kNormal, str->getLen());
//     delete[] tmp;
//     args.GetReturnValue().Set(res.ToLocalChecked() /*TODO*/);
// }

void EdBufferBuilder::Init(v8::Local<v8::Object> exports)
{
    v8::Isolate *isolate = exports->GetIsolate();

    // Prepare constructor template
    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
    tpl->SetClassName(v8::String::NewFromUtf8(isolate, "EdBufferBuilder"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "AcceptChunk", AcceptChunk);
    NODE_SET_PROTOTYPE_METHOD(tpl, "Finish", Finish);
    NODE_SET_PROTOTYPE_METHOD(tpl, "Build", Build);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(v8::String::NewFromUtf8(isolate, "EdBufferBuilder"),
                 tpl->GetFunction());
}

edcore::Buffer *EdBufferBuilder::Build()
{
    return this->_actual->Build();
}
