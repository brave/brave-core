/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_FRAME_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_FRAME_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class EdgeRequestFrame;
class EdgeCrossDOM;
class PageGraph;

class NodeFrame final : public NodeResource {
friend class PageGraph;
 public:
  NodeFrame() = delete;
  ~NodeFrame() override;
  ItemName GetItemName() const override;

  void AddInEdge(const EdgeRequestStart* const in_edge) = delete;
  void AddOutEdge(const EdgeRequestResponse* const out_edge) = delete;

  void AddInEdge(const EdgeRequestFrame* const edge);
  void AddOutEdge(const EdgeCrossDOM* const edge);

  void SetIsLocalFrame();
  void ClearIsLocalFrame();

 protected:
  NodeFrame(PageGraph* const graph, const RequestUrl url);
  GraphMLXMLList GraphMLAttributes() const override;

  bool is_local_frame_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_FRAME_H_
