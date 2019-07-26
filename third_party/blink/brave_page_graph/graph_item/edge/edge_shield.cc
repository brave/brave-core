/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_shield.h"

#include <string>

#include "base/logging.h"

#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/shield/node_shield.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/shield/node_shields.h"

namespace brave_page_graph {

EdgeShield::EdgeShield(PageGraph* const graph,
    NodeShields* const out_node, NodeShield* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeShield::~EdgeShield() {}

ItemName EdgeShield::GetItemName() const {
  return "shield";
}

bool EdgeShield::IsEdgeShield() const {
  return true;
}

}  // namespace brave_page_graph
