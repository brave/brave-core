/* Copyright (c) 2020 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_BINDING_NODE_BINDING_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_BINDING_NODE_BINDING_H_

#include <libxml/tree.h>

#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

namespace brave_page_graph {

class NodeBinding : public Node {
friend class PageGraph;
 public:
  NodeBinding() = delete;
  ~NodeBinding() override;

  Binding GetBinding() const { return binding_; }
  BindingType GetBindingType() const { return binding_type_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node)
      const override;

  bool IsNodeBinding() const override;

 protected:
  NodeBinding(PageGraph* const graph, const Binding binding,
    const BindingType binding_type);

 private:
  const Binding binding_;
  const BindingType binding_type_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeBinding> {
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return node.IsNodeBinding();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeBinding>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_BINDING_NODE_BINDING_H_
