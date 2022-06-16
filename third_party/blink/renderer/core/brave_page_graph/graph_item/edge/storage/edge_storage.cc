/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/storage/edge_storage.h"

#include <sstream>
#include <string>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"

namespace brave_page_graph {

EdgeStorage::EdgeStorage(GraphItemContext* context,
                         GraphNode* out_node,
                         GraphNode* in_node,
                         const std::string& key)
    : GraphEdge(context, out_node, in_node), key_(key) {}

EdgeStorage::~EdgeStorage() = default;

ItemName EdgeStorage::GetItemDesc() const {
  std::stringstream builder;
  builder << GraphEdge::GetItemDesc();

  if (!key_.empty()) {
    builder << " [" << key_ << "]";
  }

  return builder.str();
}

void EdgeStorage::AddGraphMLAttributes(xmlDocPtr doc,
                                       xmlNodePtr parent_node) const {
  GraphEdge::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefKey)
      ->AddValueNode(doc, parent_node, key_);
}

bool EdgeStorage::IsEdgeStorage() const {
  return true;
}

bool EdgeStorage::IsEdgeStorageClear() const {
  return false;
}

bool EdgeStorage::IsEdgeStorageDelete() const {
  return false;
}

bool EdgeStorage::IsEdgeStorageReadCall() const {
  return false;
}

bool EdgeStorage::IsEdgeStorageReadResult() const {
  return false;
}

bool EdgeStorage::IsEdgeStorageSet() const {
  return false;
}

}  // namespace brave_page_graph
