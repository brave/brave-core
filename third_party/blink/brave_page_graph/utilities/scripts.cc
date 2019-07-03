/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/utilities/scripts.h"

#include "gin/public/context_holder.h"
#include "gin/public/gin_embedders.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"

#include "v8/include/v8.h"

#include "brave/third_party/blink/brave_page_graph/page_graph.h"

using ::blink::Document;
using ::blink::ExecutionContext;
using ::blink::To;
using ::blink::ToExecutionContext;
using ::WTF::String;
using ::v8::Context;
using ::v8::Local;
using ::v8::Isolate;

namespace brave_page_graph {

static constexpr int kV8ContextPerContextDataIndex = static_cast<int>(
    gin::kPerContextDataStartIndex + gin::kEmbedderBlink);

PageGraph* GetPageGraphFromIsolate(Isolate& isolate) {
  Local<Context> context = isolate.GetCurrentContext();
  if (context.IsEmpty() == true) {
    return nullptr;
  }

  if (kV8ContextPerContextDataIndex >=
      context->GetNumberOfEmbedderDataFields()) {
    return nullptr;  // This is not a blink::ExecutionContext.
  }

  ExecutionContext* exec_context = ToExecutionContext(context);
  if (exec_context == nullptr) {
    return nullptr;
  }

  if (!exec_context->IsDocument()) {
    return nullptr;
  }
  Document* document = To<Document>(exec_context);

  return document->GetPageGraph();
}

void RegisterScriptStart(Isolate& isolate, const ScriptId script_id) {
  PageGraph* page_graph = GetPageGraphFromIsolate(isolate);
  if (page_graph == nullptr) {
    return;
  }
  page_graph->RegisterScriptExecStart(script_id);
}

void RegisterScriptEnd(Isolate& isolate, const ScriptId script_id) {
  PageGraph* page_graph = GetPageGraphFromIsolate(isolate);
  if (page_graph == nullptr) {
    return;
  }
  page_graph->RegisterScriptExecStop(script_id);
}

}  // namespace brave_page_graph
