/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;

namespace brave_page_graph {

EdgeAttribute::EdgeAttribute(PageGraph* const graph,
    NodeActor* const out_node, Node* const in_node,
    const string& name, const bool is_style) :
      Edge(graph, out_node, in_node),
      is_style_(is_style),
      name_(name) {}

const string& EdgeAttribute::GetAttributeName() const {
  return name_;
}

bool EdgeAttribute::GetIsStyle() const {
  return is_style_;
}

GraphMLXMLList EdgeAttribute::GraphMLAttributes() const {
  GraphMLXMLList attrs;
  attrs.push_back(
    GraphMLAttrDefForType(kGraphMLAttrDefKey)->ToValue(GetAttributeName()));

  if (is_style_) {
    attrs.push_back(
      GraphMLAttrDefForType(kGraphMLAttrDefIsStyle)->ToValue(true));
  }

  return attrs;
}

}  // namespace brave_page_graph
