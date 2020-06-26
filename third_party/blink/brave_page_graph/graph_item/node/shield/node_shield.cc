/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/shield/node_shield.h"

#include <string>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::string;

namespace brave_page_graph {

NodeShield::NodeShield(PageGraph* const graph, const string& type) :
      Node(graph),
      type_(type) {}

NodeShield::~NodeShield() {}

ItemName NodeShield::GetItemName() const {
  return type_ + " shield";
}

bool NodeShield::IsNodeShield() const {
  return true;
}

}  // namespace brave_page_graph
