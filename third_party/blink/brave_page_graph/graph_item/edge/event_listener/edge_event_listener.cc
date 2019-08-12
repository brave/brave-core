/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/event_listener/edge_event_listener.h"

#include <string>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_actor.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_element.h"

using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

EdgeEventListener::EdgeEventListener(const NodeHTMLElement* const out_node,
    NodeActor* const in_node, const std::string& event_type,
    const EventListenerId listener_id) :
      Edge(const_cast<NodeHTMLElement*>(out_node), in_node),
      event_type_(event_type),
      listener_id_(listener_id) {}

EdgeEventListener::EdgeEventListener(PageGraph* const graph,
    NodeHTMLElement* const out_node, NodeActor* const in_node,
    const std::string& event_type, const EventListenerId listener_id) :
      Edge(graph, out_node, in_node),
      event_type_(event_type),
      listener_id_(listener_id) {}

EdgeEventListener::~EdgeEventListener() {}

ItemName EdgeEventListener::GetItemName() const {
  return "event listener";
}

ItemDesc EdgeEventListener::GetItemDesc() const {
  return Edge::GetItemDesc()
      + " [" + event_type_ + "]"
      + " [listener id: " + to_string(listener_id_) + "]";
}

void EdgeEventListener::AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node) const {
  Edge::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefKey)
      ->AddValueNode(doc, parent_node, event_type_);
  GraphMLAttrDefForType(kGraphMLAttrDefEventListenerId)
      ->AddValueNode(doc, parent_node, listener_id_);
}

bool EdgeEventListener::IsEdgeEventListener() const {
  return true;
}

}  // namespace brave_page_graph
