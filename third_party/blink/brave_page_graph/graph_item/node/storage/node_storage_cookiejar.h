/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_STORAGE_NODE_STORAGE_COOKIEJAR_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_STORAGE_NODE_STORAGE_COOKIEJAR_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/storage/node_storage.h"

namespace brave_page_graph {

class PageGraph;

class NodeStorageCookieJar final : public NodeStorage {
friend class PageGraph;
 public:
  NodeStorageCookieJar() = delete;
  ~NodeStorageCookieJar() override;

  ItemName GetItemName() const override;

  bool IsNodeStorageCookieJar() const override;

 protected:
  NodeStorageCookieJar(PageGraph* const graph);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeStorageCookieJar> {
  static bool AllowFrom(const brave_page_graph::NodeStorage& node) {
    return node.IsNodeStorageCookieJar();
  }
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return IsA<brave_page_graph::NodeStorageCookieJar>(
        DynamicTo<brave_page_graph::NodeStorage>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeStorageCookieJar>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_STORAGE_NODE_STORAGE_COOKIEJAR_H_
