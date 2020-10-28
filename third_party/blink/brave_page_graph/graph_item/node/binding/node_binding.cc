/* Copyright (c) 2020 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/binding/node_binding.h"

#include <string>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::to_string;

namespace brave_page_graph {

NodeBinding::NodeBinding(PageGraph* const graph, const Binding binding,
    const BindingType binding_type) :
      Node(graph),
      binding_(binding),
      binding_type_(binding_type) {}

NodeBinding::~NodeBinding() {}

ItemName NodeBinding::GetItemName() const {
  return "binding";
}

ItemDesc NodeBinding::GetItemDesc() const {
  return Node::GetItemDesc() + " [" + binding_ + "]";
}

void NodeBinding::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  Node::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefBinding)
      ->AddValueNode(doc, parent_node, binding_);
  GraphMLAttrDefForType(kGraphMLAttrDefBindingType)
      ->AddValueNode(doc, parent_node, binding_type_);
}

bool NodeBinding::IsNodeBinding() const {
  return true;
}

}  // namespace brave_page_graph
