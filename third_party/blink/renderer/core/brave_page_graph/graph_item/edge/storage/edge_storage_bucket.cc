/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/storage/edge_storage_bucket.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/storage/node_storage.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/storage/node_storage_root.h"

namespace brave_page_graph {

EdgeStorageBucket::EdgeStorageBucket(GraphItemContext* context,
                                     NodeStorageRoot* out_node,
                                     NodeStorage* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeStorageBucket::~EdgeStorageBucket() = default;

ItemName EdgeStorageBucket::GetItemName() const {
  return "storage bucket";
}

bool EdgeStorageBucket::IsEdgeStorageBucket() const {
  return true;
}

}  // namespace brave_page_graph
