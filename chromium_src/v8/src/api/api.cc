/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/v8/src/api/api.cc"

#include "brave/v8/include/v8-isolate-page-graph-utils.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

namespace v8::page_graph {

ExecutingScript GetExecutingScript(Isolate* isolate, bool include_position) {
  i::Isolate* internal_isolate = reinterpret_cast<i::Isolate*>(isolate);
  return internal_isolate->GetExecutingScript(include_position);
}

std::vector<ExecutingScript> GetAllExecutingScripts(Isolate* isolate) {
  i::Isolate* internal_isolate = reinterpret_cast<i::Isolate*>(isolate);
  return internal_isolate->GetAllExecutingScripts();
}

void SetPageGraphDelegate(Isolate* isolate,
                          PageGraphDelegate* page_graph_delegate) {
  i::Isolate* internal_isolate = reinterpret_cast<i::Isolate*>(isolate);
  internal_isolate->set_page_graph_delegate(page_graph_delegate);
}

}  // namespace v8::page_graph

#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
