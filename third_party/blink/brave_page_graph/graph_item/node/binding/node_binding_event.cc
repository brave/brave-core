/* Copyright (c) 2020 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/binding/node_binding_event.h"

#include <string>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::to_string;

namespace brave_page_graph {

NodeBindingEvent::NodeBindingEvent(PageGraph* const graph,
    const BindingEvent binding_event) :
      Node(graph),
      binding_event_(binding_event) {}

NodeBindingEvent::~NodeBindingEvent() {}

ItemName NodeBindingEvent::GetItemName() const {
  return "binding event";
}

ItemDesc NodeBindingEvent::GetItemDesc() const {
  return Node::GetItemDesc() + " [" + binding_event_ + "]";
}

void NodeBindingEvent::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  Node::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefBindingEvent)
      ->AddValueNode(doc, parent_node, binding_event_);
}

bool NodeBindingEvent::IsNodeBindingEvent() const {
  return true;
}

}  // namespace brave_page_graph
