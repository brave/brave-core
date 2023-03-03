/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_TRACKER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_TRACKER_H_

#include <ostream>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"

namespace v8 {
class Isolate;
}

namespace brave_page_graph {

class NodeScript;
class PageGraphContext;

class ScriptTracker {
 public:
  using ScriptKey = std::pair<v8::Isolate*, ScriptId>;

  explicit ScriptTracker(PageGraphContext* page_graph_context);
  ~ScriptTracker();

  NodeScript* AddScriptNode(v8::Isolate* isolate,
                            ScriptId script_id,
                            const ScriptData& script_data);
  NodeScript* GetScriptNode(v8::Isolate* isolate, ScriptId script_id) const;

 private:
  PageGraphContext* page_graph_context_;
  HashMap<ScriptKey, NodeScript*> scripts_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_TRACKER_H_
