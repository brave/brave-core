/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/filter/node_filter.h"

#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

namespace brave_page_graph {

NodeFilter::NodeFilter(PageGraph* const graph) :
      Node(graph) {}

NodeFilter::~NodeFilter() {}

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
