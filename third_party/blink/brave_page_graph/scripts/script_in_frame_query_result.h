/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_IN_FRAME_QUERY_RESULT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_IN_FRAME_QUERY_RESULT_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class NodeScript;
class ScriptInFrameQuerier;

class ScriptInFrameQueryResult {
friend class ScriptInFrameQuerier;
 public:
  ~ScriptInFrameQueryResult();

  bool IsMatch() const;
  const NodeScript* GetScriptNode() const;
  blink::DOMNodeId GetFrameDOMNodeId() const;
  std::string GetFrameUrl() const;

 protected:
  ScriptInFrameQueryResult();
  ScriptInFrameQueryResult(const NodeScript* const script_node,
    const blink::DOMNodeId node_id, const std::string& url);
  const bool is_match_;
  const NodeScript* const script_node_;
  const blink::DOMNodeId frame_node_id_;
  const std::string url_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_SCRIPTS_SCRIPT_IN_FRAME_QUERY_RESULT_H_
