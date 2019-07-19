/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include "base/logging.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_dom_root.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::blink::DOMNodeId;
using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

NodeDOMRoot::NodeDOMRoot(PageGraph* const graph, const DOMNodeId node_id)
    : NodeHTMLElement(graph, node_id, "document") {}

ItemName NodeDOMRoot::GetItemName() const {
  return "DOM root #" + to_string(id_);
}

GraphMLXMLList NodeDOMRoot::GraphMLAttributes() const {
  GraphMLXMLList attrs = NodeHTML::GraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("dom root"));
  return attrs;
}

}  // namespace brave_page_graph
