/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_fingerprinting_filter.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

NodeFingerprintingFilter::NodeFingerprintingFilter(PageGraph* const graph,
    const FingerprintingRule& rule) :
      NodeFilter(graph),
      rule_(rule) {}

NodeFingerprintingFilter::~NodeFingerprintingFilter() {}

ItemName NodeFingerprintingFilter::GetItemName() const {
  return "fingerprinting filter #" + to_string(id_);
}

const FingerprintingRule& NodeFingerprintingFilter::GetRule() const {
  return rule_;
}

ItemDesc NodeFingerprintingFilter::GetDescBody() const {
  return GetItemName() + " (" + rule_.ToString() + ")";
}

GraphMLXMLList NodeFingerprintingFilter::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("fingerprinting filter"),
    GraphMLAttrDefForType(kGraphMLAttrDefPrimaryPattern)
      ->ToValue(rule_.primary_pattern),
    GraphMLAttrDefForType(kGraphMLAttrDefSecondaryPattern)
      ->ToValue(rule_.secondary_pattern),
    GraphMLAttrDefForType(kGraphMLAttrDefSource)
      ->ToValue(rule_.source),
    GraphMLAttrDefForType(kGraphMLAttrDefIncognito)
      ->ToValue(rule_.incognito)
  };
}

}  // namespace brave_page_graph
