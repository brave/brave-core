/* Copyright (c) 2020 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/binding/node_binding_event.h"

#include <string>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"

namespace brave_page_graph {

NodeBindingEvent::NodeBindingEvent(GraphItemContext* context,
                                   const BindingEvent binding_event)
    : GraphNode(context), binding_event_(binding_event) {}

NodeBindingEvent::~NodeBindingEvent() = default;

ItemName NodeBindingEvent::GetItemName() const {
  return "binding event";
}

ItemDesc NodeBindingEvent::GetItemDesc() const {
  return GraphNode::GetItemDesc() + " [" + binding_event_ + "]";
}

void NodeBindingEvent::AddGraphMLAttributes(xmlDocPtr doc,
                                            xmlNodePtr parent_node) const {
  GraphNode::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefBindingEvent)
      ->AddValueNode(doc, parent_node, binding_event_);
}

bool NodeBindingEvent::IsNodeBindingEvent() const {
  return true;
}

}  // namespace brave_page_graph
