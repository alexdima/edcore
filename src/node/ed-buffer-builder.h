#ifndef SRC_ED_BUFFER_BUILDER_H_
#define SRC_ED_BUFFER_BUILDER_H_

#include <node.h>
#include <node_object_wrap.h>

#include "../core/edcore.h"

class EdBufferBuilder : public node::ObjectWrap
{
  private:
    edcore::BufferBuilder *_actual;

    explicit EdBufferBuilder();
    ~EdBufferBuilder();

    static v8::Persistent<v8::Function> constructor;
    static void New(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void AcceptChunk(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void Finish(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void Build(const v8::FunctionCallbackInfo<v8::Value> &args);

  public:
    static void Init(v8::Local<v8::Object> exports);

    edcore::Buffer* Build();
};

#endif
