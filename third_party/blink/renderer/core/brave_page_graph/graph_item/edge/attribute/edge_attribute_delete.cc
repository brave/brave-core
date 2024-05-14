/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/attribute/edge_attribute_delete.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"

namespace brave_page_graph {

EdgeAttributeDelete::EdgeAttributeDelete(GraphItemContext* context,
                                         NodeActor* out_node,
                                         NodeHTMLElement* in_node,
                                         const FrameId& frame_id,
                                         const String& name,
                                         const bool is_style)
    : EdgeAttribute(context, out_node, in_node, frame_id, name, is_style) {}

EdgeAttributeDelete::~EdgeAttributeDelete() = default;

ItemName EdgeAttributeDelete::GetItemName() const {
  return "delete attribute";
}

bool EdgeAttributeDelete::IsEdgeAttributeDelete() const {
  return true;
}

}  // namespace brave_page_graph
