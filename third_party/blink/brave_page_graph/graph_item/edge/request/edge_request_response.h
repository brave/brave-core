/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_RESPONSE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_RESPONSE_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class Node;
class NodeResource;
class PageGraph;

class EdgeRequestResponse : public EdgeRequest {
friend class PageGraph;
 public:
  EdgeRequestResponse() = delete;
  ~EdgeRequestResponse() override;
  ItemName GetItemName() const override;
  NodeResource* GetResourceNode() const override;
  Node* GetRequestingNode() const override;

 protected:
  EdgeRequestResponse(PageGraph* const graph, NodeResource* const out_node,
    Node* const in_node, const InspectorId request_id,
    const RequestStatus status);
};

}  // brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_RESPONSE_H_
