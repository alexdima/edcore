/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef SRC_ED_BUFFER_BUILDER_H_
#define SRC_ED_BUFFER_BUILDER_H_

#include <node.h>
#include <node_object_wrap.h>

#include "../core/buffer-builder.h"

class EdBufferBuilder : public node::ObjectWrap
{
  public:
    static void Init(v8::Local<v8::Object> exports);
    edcore::Buffer *BuildBuffer();

  private:
    edcore::BufferBuilder *actual_;

    explicit EdBufferBuilder();
    ~EdBufferBuilder();

    static v8::Persistent<v8::Function> constructor;
    static void New(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void AcceptChunk(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void Finish(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void Build(const v8::FunctionCallbackInfo<v8::Value> &args);
};

#endif
