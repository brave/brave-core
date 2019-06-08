/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_FRAME_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_FRAME_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class EdgeImport;
class PageGraph;

class NodeFrame : public Node {
friend class PageGraph;
 public:
  NodeFrame() = delete;
  ~NodeFrame() override;
  ItemName GetItemName() const override;

  void AddInEdge(const Edge* const edge) = delete;
  void AddOutEdge(const EdgeImport* const edge);

 protected:
  NodeFrame(PageGraph* const graph, const blink::DOMNodeId node_id,
    const std::string& frame_url);
  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  const blink::DOMNodeId node_id_;
  const std::string frame_url_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_FRAME_H_
