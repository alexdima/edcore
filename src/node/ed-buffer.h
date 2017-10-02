/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef SRC_ED_BUFFER_H_
#define SRC_ED_BUFFER_H_

#include <node.h>
#include <node_object_wrap.h>

#include "../core/buffer.h"
#include "ed-buffer-builder.h"

class EdBuffer : public node::ObjectWrap
{
  public:
    static void Init(v8::Local<v8::Object> exports);
    static v8::Local<v8::Object> Create(v8::Isolate *isolate, const v8::Local<v8::Object> builder);

  private:
    edcore::Buffer *actual_;

    explicit EdBuffer(EdBufferBuilder *builder);
    ~EdBuffer();

    static v8::Persistent<v8::Function> constructor;
    static void New(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void GetLineCount(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void GetLineContent(const v8::FunctionCallbackInfo<v8::Value> &args);
};

#endif
