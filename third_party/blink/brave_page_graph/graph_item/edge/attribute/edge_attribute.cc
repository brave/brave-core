/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/attribute/edge_attribute.h"

#include <string>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_actor.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_element.h"

using ::std::string;

namespace brave_page_graph {

EdgeAttribute::EdgeAttribute(PageGraph* const graph,
    NodeActor* const out_node, NodeHTMLElement* const in_node,
    const string& name, const bool is_style) :
      Edge(graph, out_node, in_node),
      name_(name),
      is_style_(is_style) {}

EdgeAttribute::~EdgeAttribute() {}

ItemDesc EdgeAttribute::GetItemDesc() const {
  return Edge::GetItemDesc() + " [" + name_ + "]";
}

GraphMLXMLList EdgeAttribute::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = Edge::GetGraphMLAttributes();
  attrs.push_back(
      GraphMLAttrDefForType(kGraphMLAttrDefKey)->ToValue(name_));
  attrs.push_back(
      GraphMLAttrDefForType(kGraphMLAttrDefIsStyle)->ToValue(is_style_));
  return attrs;
}

bool EdgeAttribute::IsEdgeAttribute() const {
  return true;
}

bool EdgeAttribute::IsEdgeAttributeDelete() const {
  return false;
}

bool EdgeAttribute::IsEdgeAttributeSet() const {
  return false;
}

}  // namespace brave_page_graph
