/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_DOM_ROOT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_DOM_ROOT_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class NodeDOMRoot final : public NodeHTMLElement {
friend class PageGraph;
friend class NodeHTML;
 public:
  NodeDOMRoot() = delete;
  ItemName GetItemName() const override;

 protected:
  NodeDOMRoot(PageGraph* const graph, const blink::DOMNodeId node_id);
  GraphMLXMLList GraphMLAttributes() const override;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_DOM_ROOT_H_
