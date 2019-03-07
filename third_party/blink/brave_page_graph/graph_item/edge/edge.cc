/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include <sstream>
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;
using ::std::stringstream;
using ::std::to_string;

namespace brave_page_graph {

Edge::Edge(PageGraph* const graph, Node* const out_node, Node* const in_node) :
      GraphItem(graph),
      out_node_(out_node),
      in_node_(in_node) {}

GraphMLId Edge::GetGraphMLId() const {
  return "e" + to_string(id_);
}

GraphMLXML Edge::GetGraphMLTag() const {
  const GraphMLXMLList graphml_attributes = GraphMLAttributes();
  const bool has_graphml_attrs = graphml_attributes.size() > 0;

  stringstream builder;
  builder << "<edge id=\"" << GetGraphMLId() << "\" " <<
                    "source=\"" << out_node_->GetGraphMLId() << "\" " <<
                    "target=\"" << in_node_->GetGraphMLId() << "\"";
  if (has_graphml_attrs == false) {
    builder << "/>";
    return builder.str();
  }

  builder << ">";
  for (const GraphMLXML& elm : graphml_attributes) {
    builder << "\t" << elm;
  }
  builder << "</edge>";
  return builder.str();
}

ItemDesc Edge::GetDescPrefix() const {
  return in_node_->GetItemName() + " -> ";
}

ItemDesc Edge::GetDescSuffix() const {
  return " -> " + out_node_->GetItemName();
}

}  // namespace brave_page_graph
