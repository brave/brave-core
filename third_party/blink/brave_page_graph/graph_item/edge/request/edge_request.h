/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

class Node;
class NodeResource;
class PageGraph;

class EdgeRequest : public Edge {
friend class PageGraph;
 public:
  EdgeRequest() = delete;
  ~EdgeRequest() override;

  InspectorId GetRequestId() const { return request_id_; }
  RequestStatus GetRequestStatus() const { return request_status_; }
  RequestURL GetRequestURL() const;

  // This is just a more-semantically meaningful alias for which
  // node is the requestor and which is the resource, which will differ
  // depending on the request status (i.e. initiation, response or error).
  virtual NodeResource* GetResourceNode() const = 0;
  virtual Node* GetRequestingNode() const = 0;

  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsEdgeRequest() const override;

  virtual bool IsEdgeRequestStart() const;
  virtual bool IsEdgeRequestResponse() const;

 protected:
  EdgeRequest(PageGraph* const graph, Node* const out_node, Node* const in_node,
    const InspectorId request_id, const RequestStatus request_status);

 private:
  const InspectorId request_id_;
  const RequestStatus request_status_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeRequest> {
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return edge.IsEdgeRequest();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeRequest>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_H_
