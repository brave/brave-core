/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_delete.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

EdgeNodeDelete::EdgeNodeDelete(PageGraph* const graph,
    NodeScript* const out_node, NodeHTML* const in_node) :
      EdgeNode(graph, out_node, in_node) {}

EdgeNodeDelete::~EdgeNodeDelete() {}

ItemName EdgeNodeDelete::GetItemName() const {
  return "EdgeNodeDelete#" + to_string(id_);
}

GraphMLXMLList EdgeNodeDelete::GraphMLAttributes() const {
  return {
    graphml_attr_def_for_type(kGraphMLAttrDefEdgeType)
      ->ToValue("delete")
  };
}

}  // namespace brave_page_graph
