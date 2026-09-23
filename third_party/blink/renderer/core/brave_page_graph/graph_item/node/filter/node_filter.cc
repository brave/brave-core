/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/filter/node_filter.h"

namespace brave_page_graph {

NodeFilter::NodeFilter(GraphItemContext* context) : GraphNode(context) {}

NodeFilter::~NodeFilter() = default;

bool NodeFilter::IsNodeFilter() const {
  return true;
}

bool NodeFilter::IsNodeAdFilter() const {
  return false;
}

bool NodeFilter::IsNodeFingerprintingFilter() const {
  return false;
}

bool NodeFilter::IsNodeTrackerFilter() const {
  return false;
}

}  // namespace brave_page_graph
