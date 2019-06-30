/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_DISPATCHERS_SCRIPTS_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_DISPATCHERS_SCRIPTS_H_

#include "brave/third_party/blink/brave_page_graph/types.h"

namespace v8 {
class Isolate;
}

namespace brave_page_graph {

class PageGraph;

PageGraph* GetPageGraphFromIsolate(v8::Isolate& isolate);
void RegisterScriptStart(v8::Isolate& isolate, const ScriptId script_id);
void RegisterScriptEnd(v8::Isolate& isolate, const ScriptId script_id);

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_DISPATCHERS_SCRIPTS_H_
