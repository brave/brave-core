/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_SHIELD_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_SHIELD_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/graph_edge.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class NodeShield;
class NodeShields;

class EdgeShield : public GraphEdge {
 public:
  EdgeShield(GraphItemContext* context,
             NodeShields* out_node,
             NodeShield* in_node);
  ~EdgeShield() override;

  ItemName GetItemName() const override;

  bool IsEdgeShield() const override;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeShield> {
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return edge.IsEdgeShield();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeShield>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_SHIELD_H_
