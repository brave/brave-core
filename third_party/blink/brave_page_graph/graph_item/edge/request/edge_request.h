/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class Node;
class NodeResource;
class PageGraph;

class EdgeRequest : public Edge {
friend class PageGraph;
 public:
  EdgeRequest() = delete;
  ~EdgeRequest() override;
  RequestUrl GetRequestUrl() const;
  RequestStatus GetRequestStatus() const;
  InspectorId GetRequestId() const;

  // This is just a more-semantically meaningful alias for which
  // node is the requestor and which is the resource, which will differ
  // depending on the request status (i.e. initiation, response or error).
  virtual NodeResource* GetResourceNode() const = 0;
  virtual Node* GetRequestingNode() const = 0;

 protected:
  EdgeRequest(PageGraph* const graph, Node* const out_node,
    Node* const in_node, const InspectorId request_id,
    const RequestStatus status);
  GraphMLXMLList GraphMLAttributes() const override;

  const InspectorId request_id_;
  const RequestStatus status_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_H_
