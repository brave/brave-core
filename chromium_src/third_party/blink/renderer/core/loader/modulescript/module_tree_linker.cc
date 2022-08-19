/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

#define BRAVE_MODULE_TREE_LINKER_FETCH_DESCENDANTS                        \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                                 \
    if (CoreProbeSink::HasAgentsGlobal(CoreProbeSink::kPageGraph)) {      \
      options.SetDOMNodeId(module_script->FetchOptions().GetDOMNodeId()); \
      if (record->IsSourceTextModule()) {                                 \
        options.SetParentScriptId(record->ScriptId());                    \
      }                                                                   \
    }                                                                     \
  })

#include "src/third_party/blink/renderer/core/loader/modulescript/module_tree_linker.cc"

#undef BRAVE_MODULE_TREE_LINKER_FETCH_DESCENDANTS
