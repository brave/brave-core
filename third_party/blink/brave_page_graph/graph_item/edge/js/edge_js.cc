/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/js/edge_js.h"

#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::string;

namespace brave_page_graph {

EdgeJS::EdgeJS(PageGraph* const graph, Node* const out_node,
    Node* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeJS::~EdgeJS() {}

void EdgeJS::AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node) const {
  Edge::AddGraphMLAttributes(doc, parent_node);
}

bool EdgeJS::IsEdgeJS() const {
  return true;
}

bool EdgeJS::IsEdgeJSCall() const {
  return false;
}

bool EdgeJS::IsEdgeJSResult() const {
  return false;
}

}  // namespace brave_page_graph
