/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_storage_set.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_storage.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

EdgeStorageSet::EdgeStorageSet(PageGraph* const graph,
    NodeActor* const out_node, NodeStorage* const in_node,
    const string& key, const string& value) :
      EdgeStorage(graph, out_node, in_node, key),
      value_(value) {}

EdgeStorageSet::~EdgeStorageSet() {}

ItemName EdgeStorageSet::GetItemName() const {
  return "storage set #" + to_string(id_);
}

ItemDesc EdgeStorageSet::GetDescBody() const {
  return GetItemName() + " (" + key_ + "=" + value_ + ")";
}

GraphMLXMLList EdgeStorageSet::GraphMLAttributes() const {
  GraphMLXMLList items = EdgeStorage::GraphMLAttributes();
  items.push_back(
    GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)->ToValue("set"));
  items.push_back(
    GraphMLAttrDefForType(kGraphMLAttrDefValue)->ToValue(value_));
  return items;
}

}  // namespace brave_page_graph
