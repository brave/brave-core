/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/scripts/script_tracker.h"

#include <utility>

#include "base/debug/alias.h"
#include "base/no_destructor.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script_local.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph_context.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"

namespace brave_page_graph {

namespace {

using ScriptKey = std::pair<v8::Isolate*, ScriptId>;

// Script nodes should be accessible from multiple PageGraph instances.
HashMap<ScriptKey, NodeScriptLocal*>& GetScriptNodes() {
  static base::NoDestructor<HashMap<ScriptKey, NodeScriptLocal*>> script_nodes;
  return *script_nodes;
}

}  // namespace

ScriptTracker::ScriptTracker(PageGraphContext* page_graph_context)
    : page_graph_context_(page_graph_context) {}

ScriptTracker::~ScriptTracker() = default;

NodeScriptLocal* ScriptTracker::AddScriptNode(v8::Isolate* isolate,
                                              ScriptId script_id,
                                              const ScriptData& script_data) {
  const ScriptKey script_key{isolate, script_id};
  auto it = GetScriptNodes().find(script_key);
  if (it != GetScriptNodes().end()) {
    const auto& cached_script_data = it->value->GetScriptData();
    bool is_valid_script_data = false;
    if (script_data == cached_script_data) {
      is_valid_script_data = true;
    } else {
      if (script_data.code == cached_script_data.code) {
        if (script_data.source.is_eval && cached_script_data.source.is_eval) {
          // Simple evals can be cached and shared across v8 contexts.
          is_valid_script_data = true;
        } else if (script_data.source.location_type ==
                       blink::ScriptSourceLocationType::kJavascriptUrl &&
                   cached_script_data.source.location_type ==
                       blink::ScriptSourceLocationType::kJavascriptUrl) {
          // `javascript:` scripts can be compiled from another script or from a
          // parser.
          is_valid_script_data = true;
        }
      }
    }
    if (!is_valid_script_data) {
      const ScriptData script_data_copy = script_data;
      const ScriptData cached_script_data_copy = cached_script_data;
      DEBUG_ALIAS_FOR_CSTR(script_data_code, script_data.code.Ascii().c_str(),
                           256);
      DEBUG_ALIAS_FOR_CSTR(cached_script_data_code,
                           script_data.code.Ascii().c_str(), 256);
      base::debug::Alias(&script_data_copy);
      base::debug::Alias(&cached_script_data_copy);
      NOTREACHED() << "Script data mismatch" << " isolate: " << script_key.first
                   << " script id: " << script_key.second;
    }
    return it->value;
  }
  auto* script_node =
      page_graph_context_->AddNode<NodeScriptLocal>(script_id, script_data);
  GetScriptNodes().insert(script_key, script_node);
  return script_node;
}

NodeScriptLocal* ScriptTracker::GetScriptNode(v8::Isolate* isolate,
                                              ScriptId script_id) const {
  const ScriptKey script_key{isolate, script_id};
  auto it = GetScriptNodes().find(script_key);
  CHECK(it != GetScriptNodes().end())
      << "isolate: " << script_key.first << " script id: " << script_key.second;
  return it->value;
}

NodeScriptLocal* ScriptTracker::GetPossibleScriptNode(
    v8::Isolate* isolate,
    ScriptId script_id) const {
  const ScriptKey script_key{isolate, script_id};
  auto it = GetScriptNodes().find(script_key);
  if (it != GetScriptNodes().end()) {
    return it->value;
  }
  return nullptr;
}

}  // namespace brave_page_graph
