/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/storage/edge_storage_bucket.h"

#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/storage/node_storage.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/storage/node_storage_root.h"

namespace brave_page_graph {

EdgeStorageBucket::EdgeStorageBucket(PageGraph* const graph,
    NodeStorageRoot* const out_node, NodeStorage* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeStorageBucket::~EdgeStorageBucket() {}

ItemName EdgeStorageBucket::GetItemName() const {
  return "storage bucket";
}

bool EdgeStorageBucket::IsEdgeStorageBucket() const {
  return true;
}

}  // namespace brave_page_graph
