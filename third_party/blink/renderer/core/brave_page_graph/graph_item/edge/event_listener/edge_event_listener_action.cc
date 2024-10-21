/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_action.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

EdgeEventListenerAction::EdgeEventListenerAction(
    GraphItemContext* context,
    NodeActor* out_node,
    NodeHTMLElement* in_node,
    const FrameId& frame_id,
    const String& event_type,
    const EventListenerId listener_id,
    NodeActor* listener_script)
    : GraphEdge(context, out_node, in_node),
      frame_id_(frame_id),
      event_type_(event_type),
      listener_id_(listener_id),
      listener_script_(listener_script) {}

EdgeEventListenerAction::~EdgeEventListenerAction() = default;

ScriptId EdgeEventListenerAction::GetListenerScriptId() const {
  if (auto* node_script = blink::DynamicTo<NodeScript>(listener_script_)) {
    return node_script->GetScriptId();
  }
  return 0;
}

ItemDesc EdgeEventListenerAction::GetItemDesc() const {
  StringBuilder ts;
  ts << GraphEdge::GetItemDesc() << " [" << event_type_ << "]"
     << " [listener id: " << listener_id_ << "]"
     << " [listener script id: " << GetListenerScriptId() << "]";
  return ts.ReleaseString();
}

void EdgeEventListenerAction::AddGraphMLAttributes(
    xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  GraphEdge::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefEdgeFrameId)
      ->AddValueNode(doc, parent_node, frame_id_);
  GraphMLAttrDefForType(kGraphMLAttrDefKey)
      ->AddValueNode(doc, parent_node, event_type_);
  GraphMLAttrDefForType(kGraphMLAttrDefEventListenerId)
      ->AddValueNode(doc, parent_node, listener_id_);
  GraphMLAttrDefForType(kGraphMLAttrDefEdgeScriptId)
      ->AddValueNode(doc, parent_node, GetListenerScriptId());
}

bool EdgeEventListenerAction::IsEdgeEventListenerAction() const {
  return true;
}

bool EdgeEventListenerAction::IsEdgeEventListenerAdd() const {
  return false;
}

bool EdgeEventListenerAction::IsEdgeEventListenerRemove() const {
  return false;
}

}  // namespace brave_page_graph
