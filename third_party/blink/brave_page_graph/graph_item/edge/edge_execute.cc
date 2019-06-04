/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_execute.h"
#include <string>
#include "base/logging.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_extension.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

EdgeExecute::EdgeExecute(PageGraph* const graph,
    NodeHTMLElement* const out_node, NodeScript* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeExecute::EdgeExecute(PageGraph* const graph, NodeExtension* const out_node,
    NodeScript* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeExecute::~EdgeExecute() {}

ItemName EdgeExecute::GetItemName() const {
  return "EdgeExecute#" + to_string(id_);
}

GraphMLXMLList EdgeExecute::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->ToValue("execute")
  };
}

}  // namespace brave_page_graph
