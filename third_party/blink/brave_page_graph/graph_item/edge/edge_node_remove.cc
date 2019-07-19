/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_remove.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

EdgeNodeRemove::EdgeNodeRemove(PageGraph* const graph,
    NodeScript* const out_node, NodeHTML* const in_node) :
      EdgeNode(graph, out_node, in_node) {}

EdgeNodeRemove::~EdgeNodeRemove() {}

ItemName EdgeNodeRemove::GetItemName() const {
  return "remove node #" + ::std::to_string(id_);
}

GraphMLXMLList EdgeNodeRemove::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->ToValue("remove")
  };
}

}  // namespace brave_page_graph
