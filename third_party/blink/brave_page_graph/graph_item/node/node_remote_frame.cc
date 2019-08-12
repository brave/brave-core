/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_remote_frame.h"

#include <sstream>
#include <string>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::stringstream;
using ::std::to_string;

namespace brave_page_graph {

NodeRemoteFrame::NodeRemoteFrame(PageGraph* const graph, const RequestURL url) :
      Node(graph),
      url_(url) {}

NodeRemoteFrame::~NodeRemoteFrame() {}

ItemName NodeRemoteFrame::GetItemName() const {
  return "remote frame";
}

ItemDesc NodeRemoteFrame::GetItemDesc() const {
  stringstream builder;
  builder << Node::GetItemDesc();

  if (!url_.empty()) {
    builder << " [" << url_ << "]";
  }

  return builder.str();
}

void NodeRemoteFrame::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  Node::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefURL)
      ->AddValueNode(doc, parent_node, url_);
}

bool NodeRemoteFrame::IsNodeRemoteFrame() const {
  return true;
}

}  // namespace brave_page_graph
