/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

#include <sstream>
#include <string>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::string;
using ::std::stringstream;
using ::std::to_string;

namespace brave_page_graph {

Edge::Edge(PageGraph* const graph, Node* const out_node, Node* const in_node) :
      GraphItem(graph),
      out_node_(out_node),
      in_node_(in_node) {}

Edge::Edge(Node* const out_node, Node* const in_node) :
      out_node_(out_node),
      in_node_(in_node) {}

Edge::~Edge() {}

GraphMLId Edge::GetGraphMLId() const {
  return "e" + to_string(GetId());
}

GraphMLXML Edge::GetGraphMLTag() const {
  stringstream builder;
  builder << "<edge id=\"" << GetGraphMLId() << "\" "
          << "source=\"" << out_node_->GetGraphMLId() << "\" "
          << "target=\"" << in_node_->GetGraphMLId() << "\">";
  for (const GraphMLXML& elm : GetGraphMLAttributes()) {
    builder << elm;
  }
  builder << "</edge>";
  return builder.str();
}

GraphMLXMLList Edge::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = GraphItem::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->ToValue(GetItemName()));
  return attrs;
}

bool Edge::IsEdge() const {
  return true;
}

bool Edge::IsEdgeAttribute() const {
  return false;
}

bool Edge::IsEdgeCrossDOM() const {
  return false;
}

bool Edge::IsEdgeEventListener() const {
  return false;
}

bool Edge::IsEdgeEventListenerAction() const {
  return false;
}

bool Edge::IsEdgeExecute() const {
  return false;
}

bool Edge::IsEdgeFilter() const {
  return false;
}

bool Edge::IsEdgeHTML() const {
  return false;
}

bool Edge::IsEdgeNode() const {
  return false;
}

bool Edge::IsEdgeRequest() const {
  return false;
}

bool Edge::IsEdgeResourceBlock() const {
  return false;
}

bool Edge::IsEdgeShield() const {
  return false;
}

bool Edge::IsEdgeStorage() const {
  return false;
}

bool Edge::IsEdgeStorageBucket() const {
  return false;
}

bool Edge::IsEdgeTextChange() const {
  return false;
}

bool Edge::IsEdgeWebAPI() const {
  return false;
}

}  // namespace brave_page_graph
