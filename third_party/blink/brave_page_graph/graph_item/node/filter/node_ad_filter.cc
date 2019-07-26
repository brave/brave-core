/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/filter/node_ad_filter.h"

#include <sstream>
#include <string>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"

using ::std::stringstream;
using ::std::to_string;

namespace brave_page_graph {

NodeAdFilter::NodeAdFilter(PageGraph* const graph, const std::string& rule) :
      NodeFilter(graph),
      rule_(rule) {}

NodeAdFilter::~NodeAdFilter() {}

ItemName NodeAdFilter::GetItemName() const {
  return "ad filter";
}

ItemDesc NodeAdFilter::GetItemDesc() const {
  stringstream builder;
  builder << NodeFilter::GetItemDesc();
  if (!rule_.empty()) {
    builder << " [" << rule_ << "]";
  }
  return builder.str();
}

GraphMLXMLList NodeAdFilter::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = NodeFilter::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefRule)
      ->ToValue(rule_));
  return attrs;
}

bool NodeAdFilter::IsNodeAdFilter() const {
  return true;
}

}  // namespace brave_page_graph
