/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/event_listener/edge_event_listener.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

EdgeEventListener::EdgeEventListener(GraphItemContext* context,
                                     NodeHTMLElement* out_node,
                                     NodeActor* in_node,
                                     const String& event_type,
                                     const EventListenerId listener_id)
    : GraphEdge(context, out_node, in_node),
      event_type_(event_type),
      listener_id_(listener_id) {}

EdgeEventListener::~EdgeEventListener() = default;

ItemName EdgeEventListener::GetItemName() const {
  return "event listener";
}

ItemDesc EdgeEventListener::GetItemDesc() const {
  StringBuilder ts;
  ts << GraphEdge::GetItemDesc() << " [" << event_type_ << "]"
     << " [listener id: " << listener_id_ << "]";
  return ts.ReleaseString();
}

void EdgeEventListener::AddGraphMLAttributes(xmlDocPtr doc,
                                             xmlNodePtr parent_node) const {
  GraphEdge::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefKey)
      ->AddValueNode(doc, parent_node, event_type_);
  GraphMLAttrDefForType(kGraphMLAttrDefEventListenerId)
      ->AddValueNode(doc, parent_node, listener_id_);
}

bool EdgeEventListener::IsEdgeEventListener() const {
  return true;
}

}  // namespace brave_page_graph
