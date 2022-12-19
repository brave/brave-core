/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/scripts/script_tracker.h"

#include <map>
#include <vector>

#include "base/containers/contains.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph_context.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace brave_page_graph {

ScriptTracker::ScriptTracker(PageGraphContext* page_graph_context)
    : page_graph_context_(page_graph_context) {}

ScriptTracker::~ScriptTracker() = default;

NodeScript* ScriptTracker::AddScriptNode(v8::Isolate* isolate,
                                         ScriptId script_id,
                                         const ScriptData& script_data) {
  const ScriptKey script_key{isolate, script_id};
  auto it = scripts_.find(script_key);
  if (it != scripts_.end()) {
    CHECK(it->second->GetScriptData() == script_data)
        << "isolate: " << script_key.first
        << " script id: " << script_key.second;
    return it->second;
  }
  auto* script_node =
      page_graph_context_->AddNode<NodeScript>(script_id, script_data);
  scripts_.emplace(script_key, script_node);
  return script_node;
}

NodeScript* ScriptTracker::GetScriptNode(v8::Isolate* isolate,
                                         ScriptId script_id) const {
  const ScriptKey script_key{isolate, script_id};
  auto it = scripts_.find(script_key);
  CHECK(it != scripts_.end())
      << "isolate: " << script_key.first << " script id: " << script_key.second;
  return it->second;
}

}  // namespace brave_page_graph
