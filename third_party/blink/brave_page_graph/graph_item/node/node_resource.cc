/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"

#include <string>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_resource_block.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_response.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::to_string;

namespace brave_page_graph {

NodeResource::NodeResource(PageGraph* const graph, const RequestURL url) :
      Node(graph),
      url_(url) {}

NodeResource::~NodeResource() {}

ItemName NodeResource::GetItemName() const {
  return "resource";
}

ItemDesc NodeResource::GetItemDesc() const {
  return Node::GetItemDesc() + " [" + url_ + "]";
}

GraphMLXMLList NodeResource::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = Node::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefURL)
      ->ToValue(url_));
  return attrs;
}

bool NodeResource::IsNodeResource() const {
  return true;
}

}  // namespace brave_page_graph
