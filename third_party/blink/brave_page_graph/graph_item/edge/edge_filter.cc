/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_filter.h"

#include <string>

#include "base/logging.h"

#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/filter/node_filter.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/shield/node_shield.h"

namespace brave_page_graph {

EdgeFilter::EdgeFilter(PageGraph* const graph, NodeShield* const out_node,
    NodeFilter* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeFilter::~EdgeFilter() {}

ItemName EdgeFilter::GetItemName() const {
  return "filter";
}

bool EdgeFilter::IsEdgeFilter() const {
  return true;
}

}  // namespace brave_page_graph
