/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_parser.h"

namespace brave_page_graph {

NodeParser::NodeParser(GraphItemContext* context) : NodeActor(context) {}

NodeParser::~NodeParser() = default;

ItemName NodeParser::GetItemName() const {
  return "parser";
}

bool NodeParser::IsNodeParser() const {
  return true;
}

}  // namespace brave_page_graph
