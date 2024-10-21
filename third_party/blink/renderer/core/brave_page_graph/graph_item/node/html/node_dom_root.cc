/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_dom_root.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

using ::blink::DOMNodeId;

namespace brave_page_graph {

NodeDOMRoot::NodeDOMRoot(GraphItemContext* context,
                         const DOMNodeId dom_node_id,
                         const String& tag_name,
                         bool is_attached)
    : NodeHTMLElement(context, dom_node_id, tag_name),
      is_attached_{is_attached} {}

ItemName NodeDOMRoot::GetItemName() const {
  return "DOM root";
}

ItemDesc NodeDOMRoot::GetItemDesc() const {
  StringBuilder ts;
  ts << NodeHTMLElement::GetItemDesc();
  ts << " [is attached: " << is_attached_;
  if (!url_.empty()) {
    ts << " url: " << url_;
  }
  ts << "]";
  return ts.ReleaseString();
}

void NodeDOMRoot::AddGraphMLAttributes(xmlDocPtr doc,
                                       xmlNodePtr parent_node) const {
  NodeHTMLElement::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefURL)
      ->AddValueNode(doc, parent_node, url_);
  GraphMLAttrDefForType(kGraphMLAttrDefIsFrameAttached)
      ->AddValueNode(doc, parent_node, is_attached_);
}

bool NodeDOMRoot::IsNodeDOMRoot() const {
  return true;
}

}  // namespace brave_page_graph
