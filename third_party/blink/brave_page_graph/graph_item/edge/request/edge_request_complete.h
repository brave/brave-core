/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_COMPLETE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_COMPLETE_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_response.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class Node;
class NodeResource;
class PageGraph;

class EdgeRequestComplete final : public EdgeRequestResponse {
friend class PageGraph;
 public:
  EdgeRequestComplete() = delete;
  ~EdgeRequestComplete() override;
  ItemName GetItemName() const override;
  blink::ResourceType GetResourceType() const;

 protected:
  EdgeRequestComplete(PageGraph* const graph, NodeResource* const out_node,
    Node* const in_node, const InspectorId request_id,
    const blink::ResourceType resource_type,
    const std::string& response_header_string,
    const int64_t response_body_length);
  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  const blink::ResourceType resource_type_;

  std::string response_header_string_;
  int64_t response_body_length_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_COMPLETE_H_
