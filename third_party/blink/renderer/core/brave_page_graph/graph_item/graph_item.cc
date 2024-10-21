/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/graph_item.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/graph_item_context.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

GraphItem::GraphItem(GraphItemContext* context)
    : context_(context),
      id_(context->GetNextGraphItemId()),
      time_(base::TimeTicks::Now()) {}

GraphItem::~GraphItem() = default;

ItemDesc GraphItem::GetItemDesc() const {
  StringBuilder ts;
  ts << GetItemName() << " #" << id_;
  return ts.ReleaseString();
}

void GraphItem::AddGraphMLAttributes(xmlDocPtr doc,
                                     xmlNodePtr parent_node) const {}

bool GraphItem::IsEdge() const {
  return false;
}

bool GraphItem::IsNode() const {
  return false;
}

base::TimeDelta GraphItem::GetTimeDeltaSincePageStart() const {
  return time_ - context_->GetGraphStartTime();
}

}  // namespace brave_page_graph
