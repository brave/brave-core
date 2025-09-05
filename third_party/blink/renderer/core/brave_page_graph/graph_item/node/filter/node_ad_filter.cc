/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/filter/node_ad_filter.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"

namespace brave_page_graph {

NodeAdFilter::NodeAdFilter(GraphItemContext* context, const blink::String& rule)
    : NodeFilter(context), rule_(rule) {}

NodeAdFilter::~NodeAdFilter() = default;

ItemName NodeAdFilter::GetItemName() const {
  return "ad filter";
}

ItemDesc NodeAdFilter::GetItemDesc() const {
  if (!rule_.empty()) {
    return blink::StrCat({NodeFilter::GetItemDesc(), " [", rule_, "]"});
  }
  return NodeFilter::GetItemDesc();
}

void NodeAdFilter::AddGraphMLAttributes(xmlDocPtr doc,
                                        xmlNodePtr parent_node) const {
  NodeFilter::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefRule)
      ->AddValueNode(doc, parent_node, rule_);
}

bool NodeAdFilter::IsNodeAdFilter() const {
  return true;
}

}  // namespace brave_page_graph
