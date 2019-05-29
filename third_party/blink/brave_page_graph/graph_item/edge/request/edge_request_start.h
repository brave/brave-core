/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_START_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_START_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class Node;
class NodeResource;
class PageGraph;

class EdgeRequestStart final : public EdgeRequest {
friend class PageGraph;
 public:
  EdgeRequestStart() = delete;
  ~EdgeRequestStart() override;
  ItemName GetItemName() const override;
  RequestType GetRequestType() const;

  NodeResource* GetResourceNode() const override;
  Node* GetRequestingNode() const override;
  RequestUrl GetRequestedUrl() const;

 protected:
  EdgeRequestStart(PageGraph* const graph, Node* const out_node,
    NodeResource* const in_node, const InspectorId request_id,
    const RequestType type);
  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  const RequestType type_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_START_H_
