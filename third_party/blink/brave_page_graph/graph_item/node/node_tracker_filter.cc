/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_tracker_filter.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

NodeTrackerFilter::NodeTrackerFilter(PageGraph* const graph,
    const std::string& host) :
      NodeFilter(graph),
      host_(host) {}

NodeTrackerFilter::~NodeTrackerFilter() {}

ItemName NodeTrackerFilter::GetItemName() const {
  return "tracker filter #" + to_string(id_);
}

const std::string& NodeTrackerFilter::GetHost() const {
  return host_;
}

ItemDesc NodeTrackerFilter::GetDescBody() const {
  return GetItemName() + " (" + host_ + ")";
}

GraphMLXMLList NodeTrackerFilter::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("tracker filter"),
    GraphMLAttrDefForType(kGraphMLAttrDefHost)
      ->ToValue(host_)
  };
}

}  // namespace brave_page_graph
