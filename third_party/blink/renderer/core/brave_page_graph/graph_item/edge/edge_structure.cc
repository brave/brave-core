/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/edge_structure.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/graph_edge.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"

namespace brave_page_graph {

EdgeStructure::EdgeStructure(GraphItemContext* context,
                             GraphNode* out_node,
                             GraphNode* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeStructure::~EdgeStructure() = default;

ItemName EdgeStructure::GetItemName() const {
  return "structure";
}

bool EdgeStructure::IsEdgeStructure() const {
  return true;
}

}  // namespace brave_page_graph
