/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_frame.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_frame.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_cross_dom.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

NodeFrame::NodeFrame(PageGraph* const graph, const RequestUrl url)
    : NodeResource(graph, url),
      is_local_frame_(false) {}

NodeFrame::~NodeFrame() {}

ItemName NodeFrame::GetItemName() const {
  return "frame #" + to_string(id_);
}

void NodeFrame::AddInEdge(const EdgeRequestFrame* const edge) {
  Node::AddInEdge(edge);
}

void NodeFrame::AddOutEdge(const EdgeCrossDOM* const edge) {
  Node::AddOutEdge(edge);
}

void NodeFrame::SetIsLocalFrame() {
  is_local_frame_ = true;
}

void NodeFrame::ClearIsLocalFrame() {
  is_local_frame_ = false;
}

GraphMLXMLList NodeFrame::GraphMLAttributes() const {
  const GraphMLXML& node_type_attr = is_local_frame_ ?
      GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
        ->ToValue("local frame") :
      GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
        ->ToValue("remote frame");
  return {
    node_type_attr,
    GraphMLAttrDefForType(kGraphMLAttrDefUrl)
      ->ToValue(url_)
  };
}

}  // namespace brave_page_graph
