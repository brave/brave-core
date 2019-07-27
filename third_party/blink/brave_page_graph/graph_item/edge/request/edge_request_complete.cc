/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_complete.h"
#include <string>
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_response.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

EdgeRequestComplete::EdgeRequestComplete(PageGraph* const graph,
    NodeResource* const out_node, Node* const in_node,
    const InspectorId request_id, const blink::ResourceType resource_type,
    const std::string& response_header_string,
    const int64_t response_body_length)
    : EdgeRequestResponse(graph, out_node, in_node, request_id,
          kRequestStatusComplete),
      resource_type_(resource_type),
      response_header_string_(response_header_string),
      response_body_length_(response_body_length) {}

EdgeRequestComplete::~EdgeRequestComplete() {}

ItemName EdgeRequestComplete::GetItemName() const {
  return "request complete #" + to_string(id_);
}

blink::ResourceType EdgeRequestComplete::GetResourceType() const {
  return resource_type_;
}

ItemDesc EdgeRequestComplete::GetDescBody() const {
  return GetItemName()
    + " (" + ResourceTypeToString(resource_type_) + ")";
}

GraphMLXMLList EdgeRequestComplete::GraphMLAttributes() const {
  GraphMLXMLList attrs = EdgeRequest::GraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefResourceType)
    ->ToValue(ResourceTypeToString(resource_type_)));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefValue)
    ->ToValue(response_header_string_));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefValue)
    ->ToValue(to_string(response_body_length_)));
  return attrs;
}

}  // namespace brave_page_graph
