/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_CROSS_DOM_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_CROSS_DOM_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_insert.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class NodeDOMRoot;
class PageGraph;

class EdgeCrossDOM final : public EdgeNodeInsert {
friend class PageGraph;
 public:
  EdgeCrossDOM() = delete;
  ItemName GetItemName() const override;

 protected:
  EdgeCrossDOM(PageGraph* const graph, NodeActor* const out_node,
      NodeDOMRoot* const in_node, const blink::DOMNodeId parent_id = 0);
  GraphMLXMLList GraphMLAttributes() const override;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_CROSS_DOM_H_
