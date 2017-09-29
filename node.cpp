
#include "edcore.cpp"

#include <node.h>
#include <node_object_wrap.h>

#include "src/node/ed-buffer-builder.h"
#include "src/node/ed-buffer.h"

// class EdBuffer;

// class EdBufferBuilder : public node::ObjectWrap
// {
//   private:
//     BufferBuilder *_actual;

//     explicit EdBufferBuilder()
//     {
//         this->_actual = new BufferBuilder();
//     }

//     ~EdBufferBuilder()
//     {
//         if (this->_actual != NULL)
//         {
//             delete this->_actual;
//             this->_actual = NULL;
//         }
//     }

//     static v8::Persistent<v8::Function> constructor;

//     static void New(const v8::FunctionCallbackInfo<v8::Value> &args)
//     {
//         v8::Isolate *isolate = args.GetIsolate();

//         if (args.IsConstructCall())
//         {
//             // Invoked as constructor: `new MyObject(...)`
//             EdBufferBuilder *obj = new EdBufferBuilder();
//             obj->Wrap(args.This());
//             args.GetReturnValue().Set(args.This());
//         }
//         else
//         {
//             // Invoked as plain function `MyObject(...)`, turn into construct call.
//             const int argc = 1;
//             v8::Local<v8::Value> argv[argc] = {args[0]};
//             v8::Local<v8::Context> context = isolate->GetCurrentContext();
//             v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
//             v8::Local<v8::Object> result =
//                 cons->NewInstance(context, argc, argv).ToLocalChecked();
//             args.GetReturnValue().Set(result);
//         }
//     }

//     static void AcceptChunk(const v8::FunctionCallbackInfo<v8::Value> &args)
//     {
//         v8::Isolate *isolate = args.GetIsolate();
//         EdBufferBuilder *obj = ObjectWrap::Unwrap<EdBufferBuilder>(args.Holder());

//         v8::Local<v8::String> chunk = v8::Local<v8::String>::Cast(args[0]);
//         if (!chunk->IsString())
//         {
//             isolate->ThrowException(v8::Exception::TypeError(
//                 v8::String::NewFromUtf8(isolate, "Argument must be a string")));
//             return;
//         }

//         v8::String::Value utf16Value(chunk);

//         obj->_actual->AcceptChunk(*utf16Value, utf16Value.length());
//     }

//     static void Finish(const v8::FunctionCallbackInfo<v8::Value> &args)
//     {
//         // v8::Isolate *isolate = args.GetIsolate();
//         EdBufferBuilder *obj = ObjectWrap::Unwrap<EdBufferBuilder>(args.Holder());

//         obj->_actual->Finish();
//     }

//     static void Build(const v8::FunctionCallbackInfo<v8::Value> &args)
//     {
//         v8::Isolate *isolate = args.GetIsolate();
//         v8::Local<v8::Object> result =
//             EdBuffer::Create(args.GetIsolate(), args.Holder());
//         // const int argc = 1;
//         // v8::Local<v8::Value> argv[argc] = {args.Holder()};
//         // v8::Local<v8::Context> context = isolate->GetCurrentContext();

//         // v8::FunctionCallbackInfo info = v8::FunctionCallbackInfo::

//         //     EdBuffer::New()
//         //     // EdBufferBuilder *obj = ObjectWrap::Unwrap<EdBufferBuilder>(args.Holder());

//         //     v8::Local<v8::Function>
//         //         cons = v8::Local<v8::Function>::New(isolate, EdBuffer::constructor);
//         // v8::Local<v8::Object> result =
//         //     cons->NewInstance(context, argc, argv).ToLocalChecked();
//         args.GetReturnValue().Set(result);
//     }

//     // Build(): EdBuffer;

//     // static void GetLineContent(const v8::FunctionCallbackInfo<v8::Value> &args)
//     // {
//     //     v8::Isolate *isolate = args.GetIsolate();
//     //     EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

//     //     size_t lineNumber = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();

//     //     shared_ptr<String> str = obj->_actual->getLineContent(lineNumber);
//     //     char *tmp = new char[str->getLen()];
//     //     str->writeTo(tmp);
//     //     v8::MaybeLocal<v8::String> res = v8::String::NewFromUtf8(isolate, tmp, v8::NewStringType::kNormal, str->getLen());
//     //     delete[] tmp;
//     //     args.GetReturnValue().Set(res.ToLocalChecked() /*TODO*/);
//     // }

//   public:
//     static void Init(v8::Local<v8::Object> exports)
//     {
//         v8::Isolate *isolate = exports->GetIsolate();

//         // Prepare constructor template
//         v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
//         tpl->SetClassName(v8::String::NewFromUtf8(isolate, "EdBufferBuilder"));
//         tpl->InstanceTemplate()->SetInternalFieldCount(1);

//         // Prototype
//         NODE_SET_PROTOTYPE_METHOD(tpl, "AcceptChunk", AcceptChunk);
//         NODE_SET_PROTOTYPE_METHOD(tpl, "Finish", Finish);
//         NODE_SET_PROTOTYPE_METHOD(tpl, "Build", Build);

//         constructor.Reset(isolate, tpl->GetFunction());
//         exports->Set(v8::String::NewFromUtf8(isolate, "EdBufferBuilder"),
//                      tpl->GetFunction());
//     }

//     Buffer* Build()
//     {
//         return this->_actual->Build();
//     }
// };

// class EdBuffer : public node::ObjectWrap
// {
//   private:
//     Buffer *_actual;

//     explicit EdBuffer(EdBufferBuilder *builder)
//     {
//         this->_actual = builder->Build();
//         // NULL; //buildBufferFromFile("checker.txt");
//     }

//     ~EdBuffer()
//     {
//         if (this->_actual != NULL)
//         {
//             delete this->_actual;
//             this->_actual = NULL;
//         }
//     }

//     static v8::Persistent<v8::Function> constructor;

//     static void New(const v8::FunctionCallbackInfo<v8::Value> &args)
//     {
//         v8::Isolate *isolate = args.GetIsolate();

//         if (args.IsConstructCall())
//         {
//             v8::Local<v8::Object> arg0 = args[0]->ToObject();
//             // Invoked as constructor: `new MyObject(...)`
//             EdBufferBuilder *builder = ObjectWrap::Unwrap<EdBufferBuilder>(arg0);
//             // double value = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
//             EdBuffer *obj = new EdBuffer(builder);
//             obj->Wrap(args.This());
//             args.GetReturnValue().Set(args.This());
//         }
//         else
//         {
//             // Invoked as plain function `MyObject(...)`, turn into construct call.
//             const int argc = 1;
//             v8::Local<v8::Value> argv[argc] = {args[0]};
//             v8::Local<v8::Context> context = isolate->GetCurrentContext();
//             v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
//             v8::Local<v8::Object> result =
//                 cons->NewInstance(context, argc, argv).ToLocalChecked();
//             args.GetReturnValue().Set(result);
//         }
//     }

//     static void GetLineCount(const v8::FunctionCallbackInfo<v8::Value> &args)
//     {
//         v8::Isolate *isolate = args.GetIsolate();
//         EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

//         args.GetReturnValue().Set(v8::Number::New(isolate, obj->_actual->getLineCount()));
//     }

//     static void GetLineContent(const v8::FunctionCallbackInfo<v8::Value> &args)
//     {
//         v8::Isolate *isolate = args.GetIsolate();
//         EdBuffer *obj = ObjectWrap::Unwrap<EdBuffer>(args.Holder());

//         size_t lineNumber = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();

//         shared_ptr<String> str = obj->_actual->getLineContent(lineNumber);
//         uint16_t *tmp = new uint16_t[str->getLen()];
//         str->writeTo(tmp);
//         v8::MaybeLocal<v8::String> res = v8::String::NewFromTwoByte(isolate, tmp, v8::NewStringType::kNormal, str->getLen());
//         delete[] tmp;
//         args.GetReturnValue().Set(res.ToLocalChecked() /*TODO*/);
//     }

//   public:
//     static void Init(v8::Local<v8::Object> exports)
//     {
//         v8::Isolate *isolate = exports->GetIsolate();

//         // Prepare constructor template
//         v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
//         tpl->SetClassName(v8::String::NewFromUtf8(isolate, "EdBuffer"));
//         tpl->InstanceTemplate()->SetInternalFieldCount(1);

//         // Prototype
//         NODE_SET_PROTOTYPE_METHOD(tpl, "GetLineCount", GetLineCount);
//         NODE_SET_PROTOTYPE_METHOD(tpl, "GetLineContent", GetLineContent);

//         constructor.Reset(isolate, tpl->GetFunction());
//         exports->Set(v8::String::NewFromUtf8(isolate, "EdBuffer"),
//                      tpl->GetFunction());
//     }

//     static v8::Local<v8::Object> Create(v8::Isolate *isolate, const v8::Local<v8::Object> builder)
//     {
//         const int argc = 1;
//         v8::Local<v8::Value> argv[argc] = {builder};
//         v8::Local<v8::Context> context = isolate->GetCurrentContext();

//         v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, EdBuffer::constructor);
//         v8::Local<v8::Object> result =
//             cons->NewInstance(context, argc, argv).ToLocalChecked();
//         return result;
//     }
// };



v8::Persistent<v8::Function> EdBuffer::constructor;
v8::Persistent<v8::Function> EdBufferBuilder::constructor;

// void _createBuffer(const v8::FunctionCallbackInfo<v8::Value> &args)
// {
// }

void init(v8::Local<v8::Object> exports)
{
    // NODE_SET_METHOD(exports, "createBuffer", _createBuffer);
    EdBuffer::Init(exports);
    EdBufferBuilder::Init(exports);
}

NODE_MODULE(addon, init);
