/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_V8_INCLUDE_V8_ISOLATE_PAGE_GRAPH_UTILS_H_
#define BRAVE_V8_INCLUDE_V8_ISOLATE_PAGE_GRAPH_UTILS_H_

#include <string>
#include <vector>

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-isolate.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
namespace v8::page_graph {

struct V8_EXPORT ExecutingScript {
  int script_id = 0;
  int script_position = 0;
};

class V8_EXPORT PageGraphDelegate {
 public:
  virtual ~PageGraphDelegate() = default;

  virtual void OnEvalScriptCompiled(Isolate* isolate,
                                    const int script_id,
                                    Local<String> source) = 0;
#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH_WEBAPI_PROBES)
  virtual void OnBuiltinCall(Local<Context> context,
                             const char* builtin_name,
                             const std::vector<std::string>& args,
                             const std::string* result) = 0;
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH_WEBAPI_PROBES)
};

V8_EXPORT ExecutingScript GetExecutingScript(Isolate*,
                                             bool include_position = false);
V8_EXPORT std::vector<ExecutingScript> GetAllExecutingScripts(Isolate*);
V8_EXPORT void SetPageGraphDelegate(Isolate*,
                                    PageGraphDelegate* page_graph_delegate);

// Serializes v8::Value using Inspector Protocol internals.
V8_EXPORT Local<Value> SerializeValue(Local<Context> context,
                                      Local<Value> value);

}  // namespace v8::page_graph
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#endif  // BRAVE_V8_INCLUDE_V8_ISOLATE_PAGE_GRAPH_UTILS_H_
