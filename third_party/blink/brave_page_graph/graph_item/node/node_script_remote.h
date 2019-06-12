/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_SCRIPT_REMOTE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_SCRIPT_REMOTE_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class EdgeImport;
class PageGraph;

class NodeScriptRemote : public NodeScript {
friend class PageGraph;
 public:
  NodeScriptRemote() = delete;
  ~NodeScriptRemote() override;
  ItemName GetItemName() const override;

  void AddInEdge(const Edge* const edge) = delete;
  void AddInEdge(const EdgeImport* const edge);
  using NodeScript::AddOutEdge;

 protected:
  NodeScriptRemote(PageGraph* const graph, const NodeScript* const script_node);
  GraphMLXMLList GraphMLAttributes() const override;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_SCRIPT_REMOTE_H_
