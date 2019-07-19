/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_extensions.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

NodeExtensions::NodeExtensions(PageGraph* const graph) :
      Node(graph) {}

NodeExtensions::~NodeExtensions() {}

ItemName NodeExtensions::GetItemName() const {
  return "extensions";
}

GraphMLXMLList NodeExtensions::GraphMLAttributes() const {
  GraphMLXMLList attrs;
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("extensions"));
  return attrs;
}

ItemDesc NodeExtensions::GetDescBody() const {
  return GetItemName();
}

}  // namespace brave_page_graph
