/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_TEXT_CHANGE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_TEXT_CHANGE_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class NodeHTMLText;
class NodeScript;
class PageGraph;

class EdgeTextChange : public Edge {
friend class PageGraph;
 public:
  EdgeTextChange() = delete;
  
 protected:
  EdgeTextChange(PageGraph* const graph, NodeScript* const out_node,
    NodeHTMLText* const in_node, const std::string& new_text);
  ItemName GetItemName() const override;
  ItemName GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  const std::string new_text_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_TEXT_CHANGE_H_
