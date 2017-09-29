
#include <node.h>
#include <node_object_wrap.h>

#include "src/node/ed-buffer-builder.h"
#include "src/node/ed-buffer.h"

v8::Persistent<v8::Function> EdBuffer::constructor;
v8::Persistent<v8::Function> EdBufferBuilder::constructor;

void init(v8::Local<v8::Object> exports)
{
  // NODE_SET_METHOD(exports, "createBuffer", _createBuffer);
  EdBuffer::Init(exports);
  EdBufferBuilder::Init(exports);
}

NODE_MODULE(addon, init);
