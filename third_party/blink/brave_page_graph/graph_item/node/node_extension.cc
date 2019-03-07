/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_extension.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

NodeExtension::NodeExtension(PageGraph* const graph) :
      Node(graph) {}

NodeExtension::~NodeExtension() {}

ItemName NodeExtension::GetItemName() const {
  return "NodeExtension#" + to_string(id_);
}

GraphMLXMLList NodeExtension::GraphMLAttributes() const {
  GraphMLXMLList attrs;
  attrs.push_back(graphml_attr_def_for_type(kGraphMLAttrDefNodeType)
      ->ToValue("extension"));
  return attrs;
}

ItemDesc NodeExtension::GetDescBody() const {
  return GetItemName(); 
}

}  // namespace brave_page_graph
