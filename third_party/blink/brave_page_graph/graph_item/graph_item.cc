/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"

#include <chrono>
#include <ctime>
#include <string>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

thread_local PageGraphId ad_hoc_id_counter = 0;

/* static */
void GraphItem::StartGraphMLExport(PageGraphId id_counter) {
  ad_hoc_id_counter = id_counter;
}

GraphItem::GraphItem(PageGraph* const graph) :
    id_(++(graph->id_counter_)),
    time_(std::chrono::high_resolution_clock::now()),
    graph_(graph) {}

GraphItem::GraphItem() :
    id_(++ad_hoc_id_counter),
    time_(std::chrono::high_resolution_clock::now()),
    graph_(nullptr) {}

GraphItem::~GraphItem() {}

ItemDesc GraphItem::GetItemDesc() const {
  return GetItemName() + " #" + to_string(id_);
}

void GraphItem::AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node)
    const {
}

bool GraphItem::IsEdge() const {
  return false;
}

bool GraphItem::IsNode() const {
  return false;
}

double GraphItem::GetMicroSecSincePageStart() const {
  return std::chrono::duration_cast<std::chrono::microseconds>(
      time_ - graph_->GetTimestamp()).count();
}

}  // namespace brave_page_graph
