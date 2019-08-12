/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_RESPONSE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_RESPONSE_H_

#include <string>
#include <libxml/tree.h>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"

namespace brave_page_graph {

class Node;
class NodeResource;
class PageGraph;

class EdgeRequestResponse : public EdgeRequest {
friend class PageGraph;
 public:
  EdgeRequestResponse() = delete;
  ~EdgeRequestResponse() override;

  const std::string& GetResponseHeaderString() const {
    return response_header_string_;
  }
  int64_t GetResponseBodyLength() const { return response_body_length_; }

  NodeResource* GetResourceNode() const override;
  Node* GetRequestingNode() const override;

  ItemName GetItemName() const override;

  void AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node)
      const override;

  bool IsEdgeRequestResponse() const override;

  virtual bool IsEdgeRequestComplete() const;
  virtual bool IsEdgeRequestError() const;

 protected:
  EdgeRequestResponse(PageGraph* const graph, NodeResource* const out_node,
    Node* const in_node, const InspectorId request_id,
    const RequestStatus request_status,
    const std::string& response_header_string,
    const int64_t response_body_length);

 private:
  const std::string response_header_string_;
  const int64_t response_body_length_;
};

}  // brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeRequestResponse> {
  static bool AllowFrom(const brave_page_graph::EdgeRequest& request_edge) {
    return request_edge.IsEdgeRequestResponse();
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeRequestResponse>(
        DynamicTo<brave_page_graph::EdgeRequest>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeRequestResponse>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_RESPONSE_H_
