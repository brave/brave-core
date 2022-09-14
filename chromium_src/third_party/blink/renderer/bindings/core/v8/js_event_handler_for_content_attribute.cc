/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/bindings/core/v8/js_event_handler_for_content_attribute.h"

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

#define SetCompiledHandler(SCRIPT_STATE, FUNCTION)                       \
  SetCompiledHandler(SCRIPT_STATE, FUNCTION);                            \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                                \
    probe::RegisterPageGraphScriptCompilationFromAttr(                   \
        &event_target, function_name_, script_body_, compiled_function); \
  });

#include "src/third_party/blink/renderer/bindings/core/v8/js_event_handler_for_content_attribute.cc"

#undef SetCompiledHandler
