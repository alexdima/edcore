
#include "test.cpp"

#include <node.h>
#include <node_object_wrap.h>

class EdBuffer : public node::ObjectWrap
{
  private:
    Buffer *_actual;

    explicit EdBuffer()
    {
        this->_actual = buildBufferFromFile("checker.txt");
    }

    ~EdBuffer()
    {
        if (this->_actual != NULL)
        {
            delete this->_actual;
            this->_actual = NULL;
        }
    }

    static v8::Persistent<v8::Function> constructor;

    static void New(const v8::FunctionCallbackInfo<v8::Value> &args)
    {
        v8::Isolate *isolate = args.GetIsolate();

        if (args.IsConstructCall())
        {
            // Invoked as constructor: `new MyObject(...)`
            // double value = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
            EdBuffer *obj = new EdBuffer();
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

    static void GetLineCount(const v8::FunctionCallbackInfo<v8::Value> &args)
    {
        v8::Isolate *isolate = args.GetIsolate();
        EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

        args.GetReturnValue().Set(v8::Number::New(isolate, obj->_actual->getLineCount()));
    }

    static void GetLineContent(const v8::FunctionCallbackInfo<v8::Value> &args)
    {
        v8::Isolate *isolate = args.GetIsolate();
        EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

        size_t lineNumber = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();

        shared_ptr<String> str = obj->_actual->getLineContent(lineNumber);
        char *tmp = new char[str->getLen()];
        str->writeTo(tmp);
        v8::MaybeLocal<v8::String> res = v8::String::NewFromUtf8(isolate, tmp, v8::NewStringType::kNormal, str->getLen());
        delete[] tmp;
        args.GetReturnValue().Set(res.ToLocalChecked() /*TODO*/);
    }

  public:
    static void Init(v8::Local<v8::Object> exports)
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
};

v8::Persistent<v8::Function> EdBuffer::constructor;

// void _createBuffer(const v8::FunctionCallbackInfo<v8::Value> &args)
// {
// }

void init(v8::Local<v8::Object> exports)
{
    // NODE_SET_METHOD(exports, "createBuffer", _createBuffer);
    EdBuffer::Init(exports);
}

NODE_MODULE(addon, init);