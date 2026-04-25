/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_STORAGE_NODE_STORAGE_SESSIONSTORAGE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_STORAGE_NODE_STORAGE_SESSIONSTORAGE_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/storage/node_storage.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class NodeStorageSessionStorage final : public NodeStorage {
 public:
  explicit NodeStorageSessionStorage(GraphItemContext* context);
  ~NodeStorageSessionStorage() override;

  ItemName GetItemName() const override;

  bool IsNodeStorageSessionStorage() const override;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeStorageSessionStorage> {
  static bool AllowFrom(const brave_page_graph::NodeStorage& node) {
    return node.IsNodeStorageSessionStorage();
  }
  static bool AllowFrom(const brave_page_graph::GraphNode& node) {
    return IsA<brave_page_graph::NodeStorageSessionStorage>(
        DynamicTo<brave_page_graph::NodeStorage>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeStorageSessionStorage>(
        DynamicTo<brave_page_graph::GraphNode>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_STORAGE_NODE_STORAGE_SESSIONSTORAGE_H_
