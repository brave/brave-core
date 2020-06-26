/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_parser.h"

#include <string>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_actor.h"

using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

NodeParser::NodeParser(PageGraph* const graph) :
      NodeActor(graph) {}

NodeParser::~NodeParser() {}

ItemName NodeParser::GetItemName() const {
  return "parser";
}

bool NodeParser::IsNodeParser() const {
  return true;
}

}  // namespace brave_page_graph
