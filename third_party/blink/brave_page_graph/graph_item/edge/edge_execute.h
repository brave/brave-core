/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EXECUTE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EXECUTE_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class NodeExtension;
class NodeHTMLElement;
class NodeScript;
class PageGraph;

class EdgeExecute final : public virtual Edge {
friend class PageGraph;
 public:
  EdgeExecute() = delete;
  ~EdgeExecute() override;
  ItemName GetItemName() const override;
  ScriptId GetScriptId() const;
  ScriptType GetScriptType() const;

 protected:
  EdgeExecute(PageGraph* const graph, NodeHTMLElement* const out_node,
    NodeScript* const in_node);
  EdgeExecute(PageGraph* const graph, NodeExtension* const out_node,
    NodeScript* const in_node);
  GraphMLXMLList GraphMLAttributes() const override;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EXECUTE_H_
