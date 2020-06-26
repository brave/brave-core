/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/storage/edge_storage.h"

#include <sstream>
#include <string>


#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::stringstream;
using ::std::string;

namespace brave_page_graph {

EdgeStorage::EdgeStorage(PageGraph* const graph, Node* const out_node,
    Node* const in_node, const string& key) :
      Edge(graph, out_node, in_node),
      key_(key) {}

EdgeStorage::~EdgeStorage() {}

ItemName EdgeStorage::GetItemDesc() const {
  stringstream builder;
  builder << Edge::GetItemDesc();

  if (!key_.empty()) {
    builder << " [" << key_ << "]";
  }

  return builder.str();
}

GraphMLXMLList EdgeStorage::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = Edge::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefKey)
      ->ToValue(key_));
  return attrs;
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
