/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/bindings/core/v8/v8_script_runner.cc"

#include "brave/components/brave_page_graph/common/buildflags.h"

namespace blink {

// static
v8::MaybeLocal<v8::Script> V8ScriptRunner::CompileScript(
    ScriptState* script_state,
    const ClassicScript& classic_script,
    v8::ScriptOrigin origin,
    v8::ScriptCompiler::CompileOptions compile_options,
    v8::ScriptCompiler::NoCacheReason no_cache_reason,
    bool can_use_crowdsourced_compile_hints) {
  v8::MaybeLocal<v8::Script> result = CompileScript_ChromiumImpl(
      script_state, classic_script, origin, compile_options, no_cache_reason,
      can_use_crowdsourced_compile_hints);
#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
  if (CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph)) {
    v8::Local<v8::Script> script;
    if (result.ToLocal(&script)) {
      const auto referrer_info = ReferrerScriptInfo::FromV8HostDefinedOptions(
          script_state->GetIsolate()->GetCurrentContext(),
          origin.GetHostDefinedOptions(), classic_script.SourceUrl());
      probe::RegisterPageGraphScriptCompilation(
          ExecutionContext::From(script_state), referrer_info, classic_script,
          script);
    }
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
  return result;
}

// static
v8::MaybeLocal<v8::Module> V8ScriptRunner::CompileModule(
    v8::Isolate* isolate,
    const ModuleScriptCreationParams& params,
    const TextPosition& start_position,
    v8::ScriptCompiler::CompileOptions compile_options,
    v8::ScriptCompiler::NoCacheReason no_cache_reason,
    const ReferrerScriptInfo& referrer_info) {
  v8::MaybeLocal<v8::Module> result = CompileModule_ChromiumImpl(
      isolate, params, start_position, compile_options, no_cache_reason,
      referrer_info);
#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
  if (CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph)) {
    v8::Local<v8::Module> module;
    if (result.ToLocal(&module)) {
      probe::RegisterPageGraphModuleCompilation(
          ExecutionContext::From(isolate->GetCurrentContext()), referrer_info,
          params, module);
    }
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
  return result;
}

}  // namespace blink
