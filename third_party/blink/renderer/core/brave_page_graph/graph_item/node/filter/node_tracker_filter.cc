/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/filter/node_tracker_filter.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

NodeTrackerFilter::NodeTrackerFilter(GraphItemContext* context,
                                     const String& host)
    : NodeFilter(context), host_(host) {}

NodeTrackerFilter::~NodeTrackerFilter() = default;

ItemName NodeTrackerFilter::GetItemName() const {
  return "tracker filter";
}

ItemDesc NodeTrackerFilter::GetItemDesc() const {
  StringBuilder ts;
  ts << NodeFilter::GetItemDesc() << " [" << host_ << "]";
  return ts.ReleaseString();
}

void NodeTrackerFilter::AddGraphMLAttributes(xmlDocPtr doc,
                                             xmlNodePtr parent_node) const {
  NodeFilter::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefHost)
      ->AddValueNode(doc, parent_node, host_);
}

bool NodeTrackerFilter::IsNodeTrackerFilter() const {
  return true;
}

}  // namespace brave_page_graph
