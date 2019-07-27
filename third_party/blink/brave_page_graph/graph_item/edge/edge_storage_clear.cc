/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_storage_clear.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_storage.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

EdgeStorageClear::EdgeStorageClear(PageGraph* const graph,
    NodeScript* const out_node, NodeStorage* const in_node) :
      EdgeStorage(graph, out_node, in_node, ""){}

EdgeStorageClear::~EdgeStorageClear() {}

ItemName EdgeStorageClear::GetItemName() const {
  return "clear storage #" + to_string(id_);
}

GraphMLXMLList EdgeStorageClear::GraphMLAttributes() const {
  GraphMLXMLList items = EdgeStorage::GraphMLAttributes();
  items.push_back(
    GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)->ToValue("storage clear"));
  return items;
}

}  // namespace brave_page_graph
