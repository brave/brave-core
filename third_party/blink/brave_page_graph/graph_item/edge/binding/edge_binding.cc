/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/binding/edge_binding.h"

#include <string>

#include "base/logging.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/binding/node_binding.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/binding/node_binding_event.h"

namespace brave_page_graph {

EdgeBinding::EdgeBinding(PageGraph* const graph,
    NodeBindingEvent* const out_node, NodeBinding* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeBinding::~EdgeBinding() {}

ItemName EdgeBinding::GetItemName() const {
  return "binding";
}

bool EdgeBinding::IsEdgeBinding() const {
  return true;
}

}  // namespace brave_page_graph
