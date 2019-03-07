/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_ERROR_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_ERROR_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class Node;
class NodeResource;
class PageGraph;

class EdgeRequestError final : public EdgeRequest {
friend class PageGraph;
 public:
  EdgeRequestError() = delete;
  ~EdgeRequestError() override;
  ItemName GetItemName() const override;

  NodeResource* GetResourceNode() const override;
  Node* GetRequestingNode() const override;

 protected:
  EdgeRequestError(PageGraph* const graph, NodeResource* const out_node,
    Node* const in_node, const InspectorId request_id);
  ItemDesc GetDescBody() const override;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_ERROR_H_
