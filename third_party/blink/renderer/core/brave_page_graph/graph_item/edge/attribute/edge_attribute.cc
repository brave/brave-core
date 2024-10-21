/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/attribute/edge_attribute.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

EdgeAttribute::EdgeAttribute(GraphItemContext* context,
                             NodeActor* out_node,
                             NodeHTMLElement* in_node,
                             const FrameId& frame_id,
                             const String& name,
                             const bool is_style)
    : GraphEdge(context, out_node, in_node),
      frame_id_(frame_id),
      name_(name),
      is_style_(is_style) {}

EdgeAttribute::~EdgeAttribute() = default;

ItemDesc EdgeAttribute::GetItemDesc() const {
  StringBuilder ts;
  ts << GraphEdge::GetItemDesc() << " [" << name_ << "]";
  return ts.ReleaseString();
}

void EdgeAttribute::AddGraphMLAttributes(xmlDocPtr doc,
                                         xmlNodePtr parent_node) const {
  GraphEdge::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefEdgeFrameId)
      ->AddValueNode(doc, parent_node, frame_id_);
  GraphMLAttrDefForType(kGraphMLAttrDefKey)
      ->AddValueNode(doc, parent_node, name_);
  GraphMLAttrDefForType(kGraphMLAttrDefIsStyle)
      ->AddValueNode(doc, parent_node, is_style_);
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
