/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_ad_filter.h"
#include <sstream>
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::stringstream;
using ::std::to_string;

namespace brave_page_graph {

NodeAdFilter::NodeAdFilter(PageGraph* const graph, const std::string& rule) :
      NodeFilter(graph),
      rule_(rule) {}

NodeAdFilter::~NodeAdFilter() {}

ItemName NodeAdFilter::GetItemName() const {
  return "ad filter #" + to_string(id_);
}

const std::string& NodeAdFilter::GetRule() const {
  return rule_;
}

ItemDesc NodeAdFilter::GetDescBody() const {
  stringstream builder;
  builder << GetItemName();

  if (!rule_.empty()) {
    builder << " (" << rule_ << ")";
  }

  return builder.str();
}

GraphMLXMLList NodeAdFilter::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("ad filter"),
    GraphMLAttrDefForType(kGraphMLAttrDefRule)
      ->ToValue(rule_)
  };
}

}  // namespace brave_page_graph
