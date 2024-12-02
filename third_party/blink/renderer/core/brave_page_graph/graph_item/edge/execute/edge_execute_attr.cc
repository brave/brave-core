/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/execute/edge_execute_attr.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

EdgeExecuteAttr::EdgeExecuteAttr(GraphItemContext* context,
                                 NodeHTMLElement* out_node,
                                 NodeScript* in_node,
                                 const FrameId& frame_id,
                                 const String& attribute_name)
    : EdgeExecute(context, out_node, in_node, frame_id),
      attribute_name_(attribute_name) {}

EdgeExecuteAttr::~EdgeExecuteAttr() = default;

ItemName EdgeExecuteAttr::GetItemName() const {
  return "execute from attribute";
}

ItemDesc EdgeExecuteAttr::GetItemDesc() const {
  StringBuilder ts;
  ts << EdgeExecute::GetItemDesc() << " [" << attribute_name_ << "]";
  return ts.ReleaseString();
}

void EdgeExecuteAttr::AddGraphMLAttributes(xmlDocPtr doc,
                                           xmlNodePtr parent_node) const {
  EdgeExecute::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefAttrName)
      ->AddValueNode(doc, parent_node, attribute_name_);
}

bool EdgeExecuteAttr::IsEdgeExecuteAttr() const {
  return true;
}

}  // namespace brave_page_graph
