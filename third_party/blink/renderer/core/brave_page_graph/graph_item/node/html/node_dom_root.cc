/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_dom_root.h"

#include <sstream>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"

using ::blink::DOMNodeId;

namespace brave_page_graph {

NodeDOMRoot::NodeDOMRoot(GraphItemContext* context,
                         const DOMNodeId dom_node_id,
                         const std::string& tag_name)
    : NodeHTMLElement(context, dom_node_id, tag_name) {}

ItemName NodeDOMRoot::GetItemName() const {
  return "DOM root";
}

ItemDesc NodeDOMRoot::GetItemDesc() const {
  std::stringstream builder;
  builder << NodeHTMLElement::GetItemDesc();

  if (!url_.empty()) {
    builder << " [" << url_ << "]";
  }

  return builder.str();
}

void NodeDOMRoot::AddGraphMLAttributes(xmlDocPtr doc,
                                       xmlNodePtr parent_node) const {
  NodeHTMLElement::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefURL)
      ->AddValueNode(doc, parent_node, url_);
}

bool NodeDOMRoot::IsNodeDOMRoot() const {
  return true;
}

}  // namespace brave_page_graph
