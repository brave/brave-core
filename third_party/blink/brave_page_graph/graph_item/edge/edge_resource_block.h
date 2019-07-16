/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_RESOURCE_BLOCK_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_RESOURCE_BLOCK_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class NodeShields;
class NodeResource;
class PageGraph;

class EdgeResourceBlock : public Edge {
friend class PageGraph;
 public:
  EdgeResourceBlock() = delete;
  ~EdgeResourceBlock() override;
  ItemName GetItemName() const override;

 protected:
  EdgeResourceBlock(PageGraph* const graph, NodeShields* const out_node,
    NodeResource* const in_node, const std::string& block_type);
  GraphMLXMLList GraphMLAttributes() const override;

 private:
  const std::string block_type_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_RESOURCE_BLOCK_H_
