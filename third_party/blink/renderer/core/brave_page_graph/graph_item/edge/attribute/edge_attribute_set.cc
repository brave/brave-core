/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/attribute/edge_attribute_set.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

EdgeAttributeSet::EdgeAttributeSet(GraphItemContext* context,
                                   NodeActor* out_node,
                                   NodeHTMLElement* in_node,
                                   const FrameId& frame_id,
                                   const String& name,
                                   const String& value,
                                   const bool is_style)
    : EdgeAttribute(context, out_node, in_node, frame_id, name, is_style),
      value_(value) {}

EdgeAttributeSet::~EdgeAttributeSet() = default;

ItemName EdgeAttributeSet::GetItemName() const {
  return "set attribute";
}

ItemDesc EdgeAttributeSet::GetItemDesc() const {
  StringBuilder ts;
  ts << EdgeAttribute::GetItemDesc() << " [" << GetName() << "=" << value_
     << "]";
  return ts.ReleaseString();
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
