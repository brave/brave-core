/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_cross_dom.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_dom_root.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::blink::DOMNodeId;
using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

EdgeCrossDOM::EdgeCrossDOM(
    PageGraph* const graph,
    NodeActor* const out_node,
    NodeDOMRoot* const in_node,
    const DOMNodeId parent_id)
    : EdgeNodeInsert(graph, out_node, in_node, parent_id, 0) {}

ItemName EdgeCrossDOM::GetItemName() const {
  return "EdgeCrossDOM#" + to_string(id_);
}

GraphMLXMLList EdgeCrossDOM::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->ToValue("cross dom")
  };
}

}  // namespace brave_page_graph
