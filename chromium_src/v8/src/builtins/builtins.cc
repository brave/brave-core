/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/v8/src/builtins/builtins.cc"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH_WEBAPI_PROBES)
#include "src/builtins/builtins-utils.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH_WEBAPI_PROBES)

namespace v8 {
namespace internal {

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH_WEBAPI_PROBES)
static std::string ToPageGraphArg(Isolate* isolate, Handle<Object> object) {
#ifdef OBJECT_PRINT  // Enabled with v8_enable_object_print=true gn arg.
  std::ostringstream stream;
  Print(*object, stream);
  return stream.str();
#else   // OBJECT_PRINT
  return Object::NoSideEffectsToString(isolate, object)->ToCString().get();
#endif  // OBJECT_PRINT
}

void ReportBuiltinCallAndResponse(Isolate* isolate,
                                  const char* builtin_name,
                                  const BuiltinArguments& builtin_args,
                                  const Tagged<Object>& builtin_result) {
  HandleScope scope(isolate);
  std::vector<std::string> args;
  // Start from 1 to skip receiver arg.
  for (int arg_idx = 1; arg_idx < builtin_args.length(); ++arg_idx) {
    args.push_back(ToPageGraphArg(isolate, builtin_args.at(arg_idx)));
  }

  std::string result;
  if (builtin_result.ptr()) {
    result = ToPageGraphArg(isolate, Handle<Object>(builtin_result, isolate));
  }
  isolate->page_graph_delegate()->OnBuiltinCall(
      reinterpret_cast<v8::Isolate*>(isolate), builtin_name, args,
      builtin_result.ptr() ? &result : nullptr);
}
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH_WEBAPI_PROBES)

}  // namespace internal
}  // namespace v8
