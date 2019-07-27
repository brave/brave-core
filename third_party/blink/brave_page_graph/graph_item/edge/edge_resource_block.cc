/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_resource_block.h"
#include <string>
#include "base/logging.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_filter.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_shield.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

EdgeResourceBlock::EdgeResourceBlock(PageGraph* const graph,
    NodeFilter* const out_node, NodeResource* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeResourceBlock::EdgeResourceBlock(PageGraph* const graph,
    NodeShield* const out_node, NodeResource* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeResourceBlock::~EdgeResourceBlock() {}

ItemName EdgeResourceBlock::GetItemName() const {
  return "resource block #" + to_string(id_);
}

GraphMLXMLList EdgeResourceBlock::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->ToValue("resource block")
  };
}

}  // namespace brave_page_graph
