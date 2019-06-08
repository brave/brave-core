/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_frame.h"
#include <string>
#include "base/logging.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_import.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::blink::DOMNodeId;
using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

NodeFrame::NodeFrame(PageGraph* const graph, const DOMNodeId node_id,
  const string& frame_url) :
      Node(graph),
      node_id_(node_id),
      frame_url_(frame_url) {}

NodeFrame::~NodeFrame() {}

ItemName NodeFrame::GetItemName() const {
  return "NodeFrame#" + to_string(id_);
}

void NodeFrame::AddOutEdge(const EdgeImport* const edge) {
  Node::AddOutEdge(edge);
}

GraphMLXMLList NodeFrame::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefNodeId)
      ->ToValue(node_id_),
    GraphMLAttrDefForType(kGraphMLAttrDefUrl)
      ->ToValue(frame_url_)
  };
}

}  // namespace brave_page_graph
