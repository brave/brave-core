/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_STORAGE_EDGE_STORAGE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_STORAGE_EDGE_STORAGE_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/graph_edge.h"

namespace brave_page_graph {

class GraphNode;

class EdgeStorage : public GraphEdge {
 public:
  EdgeStorage(GraphItemContext* context,
              GraphNode* out_node,
              GraphNode* in_node,
              const std::string& key);
  ~EdgeStorage() override;

  const std::string& GetKey() const { return key_; }

  ItemName GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsEdgeStorage() const override;

  virtual bool IsEdgeStorageClear() const;
  virtual bool IsEdgeStorageDelete() const;
  virtual bool IsEdgeStorageReadCall() const;
  virtual bool IsEdgeStorageReadResult() const;
  virtual bool IsEdgeStorageSet() const;

 private:
  const std::string key_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeStorage> {
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return edge.IsEdgeStorage();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeStorage>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_STORAGE_EDGE_STORAGE_H_
