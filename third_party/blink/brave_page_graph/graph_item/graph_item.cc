/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

GraphItem::GraphItem(PageGraph* const graph) :
    graph_(graph),
    id_(graph ? ++(graph->id_counter_) : 0) {}

GraphItem::~GraphItem() {}

PageGraphId GraphItem::Id() const {
  return id_;
}

ItemDesc GraphItem::GetDescBody() const {
  return GetItemName();
}

ItemDesc GraphItem::GetDesc() const {
  return GetDescPrefix() + GetDescBody() + GetDescSuffix();
}

GraphMLXMLList GraphItem::GraphMLAttributes() const {
  return GraphMLXMLList();
}

}  // namespace brave_page_graph
