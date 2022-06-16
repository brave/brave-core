/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/storage/edge_storage_set.h"

#include <string>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/storage/node_storage.h"

namespace brave_page_graph {

EdgeStorageSet::EdgeStorageSet(GraphItemContext* context,
                               NodeScript* out_node,
                               NodeStorage* in_node,
                               const std::string& key,
                               const std::string& value)
    : EdgeStorage(context, out_node, in_node, key), value_(value) {}

EdgeStorageSet::~EdgeStorageSet() = default;

ItemName EdgeStorageSet::GetItemName() const {
  return "storage set";
}

ItemDesc EdgeStorageSet::GetItemDesc() const {
  return EdgeStorage::GetItemDesc() + " [value: " + value_ + "]";
}

void EdgeStorageSet::AddGraphMLAttributes(xmlDocPtr doc,
                                          xmlNodePtr parent_node) const {
  EdgeStorage::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefValue)
      ->AddValueNode(doc, parent_node, value_);
}

bool EdgeStorageSet::IsEdgeStorageSet() const {
  return true;
}

}  // namespace brave_page_graph
