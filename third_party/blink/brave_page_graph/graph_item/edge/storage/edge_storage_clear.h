/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_STORAGE_EDGE_STORAGE_CLEAR_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_STORAGE_EDGE_STORAGE_CLEAR_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/storage/edge_storage.h"

namespace brave_page_graph {

class NodeScript;
class NodeStorage;
class PageGraph;

class EdgeStorageClear final : public EdgeStorage {
friend class PageGraph;
 public:
  EdgeStorageClear() = delete;
  ~EdgeStorageClear() override;

  ItemName GetItemName() const override;

  bool IsEdgeStorageClear() const override;

 protected:
  EdgeStorageClear(PageGraph* const graph, NodeScript* const out_node,
    NodeStorage* const in_node);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeStorageClear> {
  static bool AllowFrom(const brave_page_graph::EdgeStorage& storage_edge) {
    return storage_edge.IsEdgeStorageClear();
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeStorageClear>(
        DynamicTo<brave_page_graph::EdgeStorage>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeStorageClear>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_STORAGE_EDGE_STORAGE_CLEAR_H_
