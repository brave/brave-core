/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/filter/node_fingerprinting_filter.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

NodeFingerprintingFilter::NodeFingerprintingFilter(
    GraphItemContext* context,
    const FingerprintingRule& rule)
    : NodeFilter(context), rule_(rule) {}

NodeFingerprintingFilter::~NodeFingerprintingFilter() = default;

ItemName NodeFingerprintingFilter::GetItemName() const {
  return "fingerprinting filter";
}

ItemDesc NodeFingerprintingFilter::GetItemDesc() const {
  StringBuilder ts;
  ts << NodeFilter::GetItemDesc() << " [" << rule_.ToString() << "]";
  return ts.ReleaseString();
}

void NodeFingerprintingFilter::AddGraphMLAttributes(
    xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  NodeFilter::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefPrimaryPattern)
      ->AddValueNode(doc, parent_node, rule_.primary_pattern);
  GraphMLAttrDefForType(kGraphMLAttrDefSecondaryPattern)
      ->AddValueNode(doc, parent_node, rule_.secondary_pattern);
  GraphMLAttrDefForType(kGraphMLAttrDefSource)
      ->AddValueNode(doc, parent_node, rule_.source);
  GraphMLAttrDefForType(kGraphMLAttrDefIncognito)
      ->AddValueNode(doc, parent_node, rule_.incognito);
}

bool NodeFingerprintingFilter::IsNodeFingerprintingFilter() const {
  return true;
}

}  // namespace brave_page_graph
