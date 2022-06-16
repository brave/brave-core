/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/attribute/edge_attribute_set.h"

#include <string>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/attribute/edge_attribute.h"

namespace brave_page_graph {

EdgeAttributeSet::EdgeAttributeSet(GraphItemContext* context,
                                   NodeActor* out_node,
                                   NodeHTMLElement* in_node,
                                   const std::string& name,
                                   const std::string& value,
                                   const bool is_style)
    : EdgeAttribute(context, out_node, in_node, name, is_style),
      value_(value) {}

EdgeAttributeSet::~EdgeAttributeSet() = default;

ItemName EdgeAttributeSet::GetItemName() const {
  return "set attribute";
}

ItemDesc EdgeAttributeSet::GetItemDesc() const {
  return EdgeAttribute::GetItemDesc() + " [" + GetName() + "=" + value_ + "]";
}

void EdgeAttributeSet::AddGraphMLAttributes(xmlDocPtr doc,
                                            xmlNodePtr parent_node) const {
  EdgeAttribute::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefValue)
      ->AddValueNode(doc, parent_node, value_);
}

bool EdgeAttributeSet::IsEdgeAttributeSet() const {
  return true;
}

}  // namespace brave_page_graph
