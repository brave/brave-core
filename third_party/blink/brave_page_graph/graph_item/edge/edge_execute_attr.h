/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EXECUTE_ATTR_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EXECUTE_ATTR_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_execute.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class NodeHTMLElement;
class NodeScript;
class PageGraph;

class EdgeExecuteAttr : public EdgeExecute {
friend class PageGraph;
 public:
  EdgeExecuteAttr() = delete;
  ~EdgeExecuteAttr() override;
  ItemName GetItemName() const override;

 protected:
  EdgeExecuteAttr(PageGraph* const graph, NodeHTMLElement* const out_node,
    NodeScript* const in_node, const std::string& attr);

  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  const std::string attr_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EXECUTE_ATTR_H_
