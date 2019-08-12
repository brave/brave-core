/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/storage/edge_storage_read_result.h"

#include <string>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/storage/node_storage.h"

using ::std::string;

namespace brave_page_graph {

EdgeStorageReadResult::EdgeStorageReadResult(PageGraph* const graph,
    NodeStorage* const out_node, NodeScript* const in_node,
    const string& key, const string& value) :
      EdgeStorage(graph, out_node, in_node, key),
      value_(value) {}

EdgeStorageReadResult::~EdgeStorageReadResult() {}

ItemName EdgeStorageReadResult::GetItemName() const {
  return "storage read result";
}

ItemDesc EdgeStorageReadResult::GetItemDesc() const {
  return EdgeStorage::GetItemDesc() + " [value: " + value_ + "]";
}

void EdgeStorageReadResult::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  EdgeStorage::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefValue)
      ->AddValueNode(doc, parent_node, value_);
}

bool EdgeStorageReadResult::IsEdgeStorageReadResult() const {
  return true;
}

}  // namespace brave_page_graph
