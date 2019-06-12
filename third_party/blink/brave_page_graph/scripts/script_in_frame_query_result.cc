/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/scripts/script_in_frame_query_result.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/logging.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::blink::DOMNodeId;
using ::std::string;

namespace brave_page_graph {

ScriptInFrameQueryResult::ScriptInFrameQueryResult() :
      is_match_(false),
      script_node_(nullptr),
      frame_node_id_(0),
      url_("") {}

ScriptInFrameQueryResult::ScriptInFrameQueryResult(
    const NodeScript* const script_node, const DOMNodeId frame_node_id,
    const string& url) :
      is_match_(true),
      script_node_(script_node),
      frame_node_id_(frame_node_id),
      url_(url) {}

ScriptInFrameQueryResult::~ScriptInFrameQueryResult() {}

bool ScriptInFrameQueryResult::IsMatch() const {
  return is_match_;
}

const NodeScript* ScriptInFrameQueryResult::GetScriptNode() const {
  LOG_ASSERT(is_match_ == true);
  return script_node_;
}

DOMNodeId ScriptInFrameQueryResult::GetFrameDOMNodeId() const {
  LOG_ASSERT(is_match_ == true);
  return frame_node_id_;
}

string ScriptInFrameQueryResult::GetFrameUrl() const {
  LOG_ASSERT(is_match_ == true);
  return url_;
}

}  // namespace brave_page_graph
