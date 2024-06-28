/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_SCRIPT_RUNNER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_SCRIPT_RUNNER_H_

#include "v8/include/v8.h"

#define CompileScript                                                 \
  CompileScript(ScriptState*, const ClassicScript&, v8::ScriptOrigin, \
                v8::ScriptCompiler::CompileOptions,                   \
                v8::ScriptCompiler::NoCacheReason,                    \
                bool can_use_crowdsourced_compile_hints = false);     \
  static v8::MaybeLocal<v8::Script> CompileScript_ChromiumImpl

#define CompileModule                                                          \
  CompileModule(v8::Isolate*, const ModuleScriptCreationParams&,               \
                const WTF::TextPosition&, v8::ScriptCompiler::CompileOptions,  \
                v8::ScriptCompiler::NoCacheReason, const ReferrerScriptInfo&); \
  static v8::MaybeLocal<v8::Module> CompileModule_ChromiumImpl

#include "src/third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"  // IWYU pragma: export

#undef CompileModule
#undef CompileScript

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_SCRIPT_RUNNER_H_
