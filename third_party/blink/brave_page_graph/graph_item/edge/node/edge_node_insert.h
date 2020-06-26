/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_INSERT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_INSERT_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node.h"

namespace brave_page_graph {

class NodeActor;
class NodeHTML;
class NodeHTMLElement;
class PageGraph;

class EdgeNodeInsert final : public EdgeNode {
friend class PageGraph;
 public:
  EdgeNodeInsert() = delete;
  ~EdgeNodeInsert() override;

  NodeHTMLElement* GetParentNode() const;
  NodeHTML* GetPriorSiblingNode() const;

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsEdgeNodeInsert() const override;

 protected:
  EdgeNodeInsert(PageGraph* const graph, NodeActor* const out_node,
    NodeHTML* const in_node, const blink::DOMNodeId parent_node_id = 0,
    const blink::DOMNodeId prior_sibling_node_id = 0);

 private:
  const blink::DOMNodeId parent_node_id_;
  const blink::DOMNodeId prior_sibling_node_id_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeNodeInsert> {
  static bool AllowFrom(const brave_page_graph::EdgeNode& edge) {
    return edge.IsEdgeNodeInsert();
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeNodeInsert>(
        DynamicTo<brave_page_graph::EdgeNode>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeNodeInsert>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_INSERT_H_
