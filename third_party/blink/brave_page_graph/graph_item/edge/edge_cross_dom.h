/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_CROSS_DOM_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_CROSS_DOM_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

class NodeDOMRoot;
class NodeFrameOwner;
class PageGraph;

class EdgeCrossDOM final : public Edge {
friend class PageGraph;
 public:
  EdgeCrossDOM() = delete;
  ~EdgeCrossDOM() override;

  ItemName GetItemName() const override;

  bool IsEdgeCrossDOM() const override;

 protected:
  EdgeCrossDOM(PageGraph* const graph, NodeDOMRoot* const out_node,
      NodeDOMRoot* const in_node);
  EdgeCrossDOM(PageGraph* const graph, NodeFrameOwner* const out_node,
      NodeDOMRoot* const in_node);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeCrossDOM> {
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return edge.IsEdgeCrossDOM();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeCrossDOM>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_CROSS_DOM_H_
