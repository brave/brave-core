/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/v8/src/execution/isolate.cc"

namespace v8 {
namespace internal {

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
namespace {

std::vector<v8::page_graph::ExecutingScript>
GetExecutingScriptsImpl(Isolate* isolate, bool all, bool include_position) {
  std::vector<v8::page_graph::ExecutingScript> result;
  JavaScriptStackFrameIterator it(isolate);
  while (!it.done()) {
    JavaScriptFrame* frame = it.frame();
    std::vector<Tagged<SharedFunctionInfo>> functions;
    frame->GetFunctions(&functions);
    for (const auto& shared : functions) {
      Tagged<Object> maybe_script = shared->script();
      if (!IsScript(maybe_script)) {
        continue;
      }

      const int script_id = Cast<Script>(maybe_script)->id();
      if (script_id <= 0) {
        continue;
      }

      int script_position = 0;
      if (include_position && !isolate->has_exception()) {
        Handle<SharedFunctionInfo> shared_handle(shared, isolate);
        SharedFunctionInfo::EnsureSourcePositionsAvailable(isolate,
                                                           shared_handle);
        script_position = frame->position();
      }

      result.emplace_back(
          v8::page_graph::ExecutingScript{script_id, script_position});
      if (!all) {
        return result;
      }
    }
    it.Advance();
  }
  return result;
}

}  // namespace

v8::page_graph::ExecutingScript Isolate::GetExecutingScript(
    bool include_position) {
  auto executing_scripts =
      GetExecutingScriptsImpl(this, false, include_position);
  if (!executing_scripts.empty()) {
    return std::move(executing_scripts[0]);
  }
  return v8::page_graph::ExecutingScript();
}

std::vector<v8::page_graph::ExecutingScript> Isolate::GetAllExecutingScripts() {
  return GetExecutingScriptsImpl(this, true, true);
}
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

}  // namespace internal
}  // namespace v8
