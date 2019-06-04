/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute_set.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;

namespace brave_page_graph {

EdgeAttributeSet::EdgeAttributeSet(PageGraph* const graph,
    NodeActor* const out_node, NodeHTMLElement* const in_node,
    const string& name, const string& value, const bool is_style) :
      EdgeAttribute(graph, out_node, in_node, name, is_style),
      value_(value) {}

EdgeAttributeSet::~EdgeAttributeSet() {}

ItemName EdgeAttributeSet::GetItemName() const {
  return "EdgeAttributeSet#" + ::std::to_string(id_);
}

const string& EdgeAttributeSet::AttributeValue() const {
  return value_;
}

ItemDesc EdgeAttributeSet::GetDescBody() const {
  return GetItemName() + " [" + GetAttributeName() + "=" + AttributeValue() + "]";
}

GraphMLXMLList EdgeAttributeSet::GraphMLAttributes() const {
  GraphMLXMLList attrs = EdgeAttribute::GraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefValue)
      ->ToValue(AttributeValue()));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->ToValue("attr set"));
  return attrs;
}

}  // namespace brave_page_graph
