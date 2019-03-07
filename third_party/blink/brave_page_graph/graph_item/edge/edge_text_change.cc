/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_text_change.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_text.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;

namespace brave_page_graph {

EdgeTextChange::EdgeTextChange(PageGraph* const graph,
    NodeScript* const out_node, NodeHTMLText* const in_node,
    const std::string& new_text) :
      Edge(graph, out_node, in_node),
      new_text_(new_text) {}

ItemName EdgeTextChange::GetItemName() const {
  return "EdgeTextChange#" + ::std::to_string(id_);
}

ItemName EdgeTextChange::GetDescBody() const {
  return GetItemName() + " [key:" + new_text_ + "]";
}

GraphMLXMLList EdgeTextChange::GraphMLAttributes() const {
  return {
    graphml_attr_def_for_type(kGraphMLAttrDefEdgeType)
      ->ToValue("text change"),
    graphml_attr_def_for_type(kGraphMLAttrDefValue)
      ->ToValue(new_text_)
  };
}

}  // namespace brave_page_graph
