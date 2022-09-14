/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/execute/edge_execute.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/node_extensions.h"

namespace brave_page_graph {

EdgeExecute::EdgeExecute(GraphItemContext* context,
                         NodeHTMLElement* out_node,
                         NodeScript* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeExecute::EdgeExecute(GraphItemContext* context,
                         NodeExtensions* out_node,
                         NodeScript* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeExecute::EdgeExecute(GraphItemContext* context,
                         NodeScript* out_node,
                         NodeScript* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeExecute::~EdgeExecute() = default;

ItemName EdgeExecute::GetItemName() const {
  return "execute";
}

bool EdgeExecute::IsEdgeExecute() const {
  return true;
}

bool EdgeExecute::IsEdgeExecuteAttr() const {
  return false;
}

}  // namespace brave_page_graph
