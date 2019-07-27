/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_error.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_response.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

EdgeRequestError::EdgeRequestError(PageGraph* const graph,
    NodeResource* const out_node, Node* const in_node,
    const InspectorId request_id,
    const std::string& response_header_string,
    const int64_t response_body_length)
    : EdgeRequestResponse(graph, out_node, in_node, request_id,
        kRequestStatusError),
      response_header_string_(response_header_string),
      response_body_length_(response_body_length) {}

EdgeRequestError::~EdgeRequestError() {}

ItemName EdgeRequestError::GetItemName() const {
  return "request error #" + to_string(id_);
}

ItemDesc EdgeRequestError::GetDescBody() const {
  return GetItemName();
}

GraphMLXMLList EdgeRequestError::GraphMLAttributes() const {
  GraphMLXMLList attrs = EdgeRequest::GraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefValue)
    ->ToValue(response_header_string_));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefValue)
    ->ToValue(to_string(response_body_length_)));
  return attrs;
}

}  // namespace brave_page_graph
