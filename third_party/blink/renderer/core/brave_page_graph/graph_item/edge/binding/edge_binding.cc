/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/binding/edge_binding.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/binding/node_binding.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/binding/node_binding_event.h"

namespace brave_page_graph {

EdgeBinding::EdgeBinding(GraphItemContext* context,
                         NodeBindingEvent* out_node,
                         NodeBinding* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeBinding::~EdgeBinding() = default;

ItemName EdgeBinding::GetItemName() const {
  return "binding";
}

bool EdgeBinding::IsEdgeBinding() const {
  return true;
}

}  // namespace brave_page_graph
