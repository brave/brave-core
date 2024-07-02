/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_page_graph/common/buildflags.h"

#define BRAVE_COMPILER_GET_FUNCTION_FROM_EVAL                   \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                       \
    auto* page_graph_delegate = isolate->page_graph_delegate(); \
    if (V8_UNLIKELY(page_graph_delegate)) {                     \
      Tagged<Object> maybe_script = result->shared()->script(); \
      if (IsScript(maybe_script)) {                             \
        const int script_id = Cast<Script>(maybe_script)->id(); \
        page_graph_delegate->OnEvalScriptCompiled(              \
            reinterpret_cast<v8::Isolate*>(isolate), script_id, \
            v8::Utils::ToLocal(source));                        \
      }                                                         \
    }                                                           \
  })

#include "src/v8/src/codegen/compiler.cc"

#undef BRAVE_COMPILER_GET_FUNCTION_FROM_EVAL
