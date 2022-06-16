/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/graph_item.h"

#include <string>

#include "base/strings/string_number_conversions.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/graph_item_context.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

namespace brave_page_graph {

GraphItem::GraphItem(GraphItemContext* context)
    : context_(context),
      id_(context->GetNextGraphItemId()),
      time_(base::TimeTicks::Now()) {}

GraphItem::~GraphItem() = default;

ItemDesc GraphItem::GetItemDesc() const {
  return GetItemName() + " #" + base::NumberToString(id_);
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
