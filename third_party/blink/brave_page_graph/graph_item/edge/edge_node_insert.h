/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_NODE_INSERT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_NODE_INSERT_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

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
  ItemName GetItemName() const override;

  NodeHTMLElement* GetParentNode() const;
  NodeHTML* GetPriorSiblingNode() const;

 protected:
  EdgeNodeInsert(PageGraph* const graph, NodeActor* const out_node,
    NodeHTML* const in_node, const blink::DOMNodeId parent_id = 0,
    const blink::DOMNodeId prior_sibling_id = 0);
  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  const blink::DOMNodeId parent_id_;
  const blink::DOMNodeId prior_sibling_id_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_NODE_INSERT_H_
