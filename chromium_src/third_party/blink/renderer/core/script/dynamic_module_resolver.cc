/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "brave/v8/include/v8-isolate-page-graph-utils.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

#define BRAVE_DYNAMIC_MODULE_RESOLVER_RESOLVE_DYNAMICALLY            \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                            \
    if (CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph)) { \
      options.SetDOMNodeId(referrer_info.GetDOMNodeId());            \
      options.SetParentScriptId(                                     \
          v8::page_graph::GetExecutingScript(                        \
              modulator_->GetScriptState()->GetIsolate())            \
              .script_id);                                           \
    }                                                                \
  })

#include "src/third_party/blink/renderer/core/script/dynamic_module_resolver.cc"

#undef BRAVE_DYNAMIC_MODULE_RESOLVER_RESOLVE_DYNAMICALLY
