/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_TRACKER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_TRACKER_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"

namespace v8 {
class Isolate;
}

namespace brave_page_graph {

class NodeScriptLocal;
class PageGraphContext;

class ScriptTracker {
 public:
  explicit ScriptTracker(PageGraphContext* page_graph_context);
  ~ScriptTracker();

  NodeScriptLocal* AddScriptNode(v8::Isolate* isolate,
                                 ScriptId script_id,
                                 const ScriptData& script_data);
  NodeScriptLocal* GetScriptNode(v8::Isolate* isolate,
                                 ScriptId script_id) const;
  NodeScriptLocal* GetPossibleScriptNode(v8::Isolate* isolate,
                                         ScriptId script_id) const;

 private:
  PageGraphContext* page_graph_context_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_TRACKER_H_
