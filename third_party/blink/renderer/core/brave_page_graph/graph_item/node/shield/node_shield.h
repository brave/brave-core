/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_SHIELD_NODE_SHIELD_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_SHIELD_NODE_SHIELD_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"

namespace brave_page_graph {

class NodeShield final : public GraphNode {
 public:
  NodeShield(GraphItemContext* context, const std::string& type);

  ~NodeShield() override;

  const std::string& Type() { return type_; }

  ItemName GetItemName() const override;

  bool IsNodeShield() const override;

 private:
  const std::string type_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeShield> {
  static bool AllowFrom(const brave_page_graph::GraphNode& node) {
    return node.IsNodeShield();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeShield>(
        DynamicTo<brave_page_graph::GraphNode>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_SHIELD_NODE_SHIELD_H_
