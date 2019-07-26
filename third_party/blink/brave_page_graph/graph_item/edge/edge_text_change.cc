/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_text_change.h"

#include <string>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_text.h"

using ::std::string;

namespace brave_page_graph {

EdgeTextChange::EdgeTextChange(PageGraph* const graph,
    NodeScript* const out_node, NodeHTMLText* const in_node,
    const std::string& text) :
      Edge(graph, out_node, in_node),
      text_(text) {}

ItemName EdgeTextChange::GetItemName() const {
  return "text change";
}

ItemName EdgeTextChange::GetItemDesc() const {
  return Edge::GetItemDesc() + " [" + text_ + "]";
}

GraphMLXMLList EdgeTextChange::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = Edge::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefValue)
      ->ToValue(text_));
  return attrs;
}

bool EdgeTextChange::IsEdgeTextChange() const {
  return true;
}

}  // namespace brave_page_graph
