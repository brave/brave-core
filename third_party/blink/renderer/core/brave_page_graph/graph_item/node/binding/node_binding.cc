/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/binding/node_binding.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

namespace brave_page_graph {

NodeBinding::NodeBinding(GraphItemContext* context,
                         const Binding binding,
                         const BindingType binding_type)
    : GraphNode(context), binding_(binding), binding_type_(binding_type) {}

NodeBinding::~NodeBinding() = default;

ItemName NodeBinding::GetItemName() const {
  return "binding";
}

ItemDesc NodeBinding::GetItemDesc() const {
  return GraphNode::GetItemDesc() + " [" + binding_ + "]";
}

void NodeBinding::AddGraphMLAttributes(xmlDocPtr doc,
                                       xmlNodePtr parent_node) const {
  GraphNode::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefBinding)
      ->AddValueNode(doc, parent_node, binding_);
  GraphMLAttrDefForType(kGraphMLAttrDefBindingType)
      ->AddValueNode(doc, parent_node, binding_type_);
}

bool NodeBinding::IsNodeBinding() const {
  return true;
}

}  // namespace brave_page_graph
