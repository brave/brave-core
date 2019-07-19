/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_response.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

NodeResource::NodeResource(PageGraph* const graph, const RequestUrl url) :
      Node(graph),
      url_(url) {}

NodeResource::~NodeResource() {}

ItemName NodeResource::GetItemName() const {
  return "resource #" + to_string(id_);
}

RequestUrl NodeResource::GetUrl() const {
  return url_;
}

void NodeResource::AddInEdge(const EdgeRequestStart* const in_edge) {
  Node::AddInEdge(in_edge);
}

void NodeResource::AddOutEdge(const EdgeRequestResponse* const out_edge) {
  Node::AddOutEdge(out_edge);
}

ItemDesc NodeResource::GetDescBody() const {
  return GetItemName() + " (" + url_ + ")";
}

GraphMLXMLList NodeResource::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("resource"),
    GraphMLAttrDefForType(kGraphMLAttrDefUrl)
      ->ToValue(url_)
  };
}

}  // namespace brave_page_graph
