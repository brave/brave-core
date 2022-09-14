/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/edge_filter.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/filter/node_filter.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/shield/node_shield.h"

namespace brave_page_graph {

EdgeFilter::EdgeFilter(GraphItemContext* context,
                       NodeShield* out_node,
                       NodeFilter* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeFilter::~EdgeFilter() = default;

ItemName EdgeFilter::GetItemName() const {
  return "filter";
}

bool EdgeFilter::IsEdgeFilter() const {
  return true;
}

}  // namespace brave_page_graph
