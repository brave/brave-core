/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script_remote.h"
#include <string>
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_import.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::blink::DOMNodeId;
using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

NodeScriptRemote::NodeScriptRemote(PageGraph* const graph,
    const NodeScript* const script_node) :
      NodeScript(graph, script_node->GetScriptId(),
        script_node->GetScriptType(), script_node->GetUrl()) {}

NodeScriptRemote::~NodeScriptRemote() {}

ItemName NodeScriptRemote::GetItemName() const {
  return "NodeScriptRemote#" + to_string(id_);
}

void NodeScriptRemote::AddInEdge(const EdgeImport* const edge) {
  Node::AddOutEdge(edge);
}

GraphMLXMLList NodeScriptRemote::GraphMLAttributes() const {
  GraphMLXMLList attrs;
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("cross frame script"));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefScriptId)
      ->ToValue(script_id_));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefScriptType)
      ->ToValue(ScriptTypeToString(type_)));

  if (IsInline() == false) {
    attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefUrl)
      ->ToValue(url_));
  }

  return attrs;
}

}  // namespace brave_page_graph
