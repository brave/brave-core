/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_html.h"
#include <ostream>
#include <sstream>
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;
using ::std::stringstream;
using ::std::to_string;

namespace brave_page_graph {

EdgeHTML::EdgeHTML(const NodeHTMLElement* const out_node,
    NodeHTML* const in_node) :
      Edge(const_cast<NodeHTMLElement*>(out_node), in_node) {}

EdgeHTML::EdgeHTML(PageGraph* const graph, NodeHTMLElement* const out_node,
    NodeHTML* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeHTML::~EdgeHTML() {}

ItemName EdgeHTML::GetItemName() const {
  return "structure #" + to_string(id_);
}

GraphMLXMLList EdgeHTML::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->ToValue("structure")
  };
}

}  // namespace brave_page_graph
