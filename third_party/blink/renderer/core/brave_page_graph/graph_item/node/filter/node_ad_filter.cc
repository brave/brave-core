/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/filter/node_ad_filter.h"

#include <sstream>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

namespace brave_page_graph {

NodeAdFilter::NodeAdFilter(GraphItemContext* context, const std::string& rule)
    : NodeFilter(context), rule_(rule) {}

NodeAdFilter::~NodeAdFilter() = default;

ItemName NodeAdFilter::GetItemName() const {
  return "ad filter";
}

ItemDesc NodeAdFilter::GetItemDesc() const {
  std::stringstream builder;
  builder << NodeFilter::GetItemDesc();
  if (!rule_.empty()) {
    builder << " [" << rule_ << "]";
  }
  return builder.str();
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
