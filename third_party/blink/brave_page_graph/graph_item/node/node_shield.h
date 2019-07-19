/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_SHIELD_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_SHIELD_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class PageGraph;

class NodeShield final : public Node {
friend class PageGraph;
 public:
  NodeShield() = delete;
  ~NodeShield() override;
  ItemName GetItemName() const override;

  bool IsNodeActor() const override { return false; }

  const std::string& Type() { return type_; }

 protected:
  NodeShield(PageGraph* const graph, const std::string& type);
  GraphMLXMLList GraphMLAttributes() const override;

 private:
  const std::string type_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_SHIELDS_H_
