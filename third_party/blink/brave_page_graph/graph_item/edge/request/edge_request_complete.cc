/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_complete.h"
#include <string>
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

EdgeRequestComplete::EdgeRequestComplete(PageGraph* const graph,
    NodeResource* const out_node, Node* const in_node,
    const InspectorId request_id, const blink::ResourceType resource_type) : 
      EdgeRequest(graph, out_node, in_node, request_id, kRequestStatusComplete),
      resource_type_(resource_type) {}

EdgeRequestComplete::~EdgeRequestComplete() {}

ItemName EdgeRequestComplete::GetItemName() const {
  return "EdgeRequestComplete#" + to_string(id_);
}

blink::ResourceType EdgeRequestComplete::GetResourceType() const {
  return resource_type_;
}

NodeResource* EdgeRequestComplete::GetResourceNode() const {
  return static_cast<NodeResource*>(out_node_);
}

Node* EdgeRequestComplete::GetRequestingNode() const {
  return in_node_;
}

ItemDesc EdgeRequestComplete::GetDescBody() const {
  return GetItemName() + "[type: " + resource_type_to_string(resource_type_) + "]";
}

GraphMLXMLList EdgeRequestComplete::GraphMLAttributes() const {
  GraphMLXMLList attrs = EdgeRequest::GraphMLAttributes();
  attrs.push_back(graphml_attr_def_for_type(kGraphMLAttrDefResourceType)
    ->ToValue(resource_type_to_string(resource_type_)));
  return attrs;
}


}  // namespace brave_page_graph
