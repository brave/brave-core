/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_STORAGE_NODE_STORAGE_ROOT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_STORAGE_NODE_STORAGE_ROOT_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

namespace brave_page_graph {

class PageGraph;

class NodeStorageRoot final : public Node {
friend class PageGraph;
 public:
  NodeStorageRoot() = delete;
  ~NodeStorageRoot() override;

  ItemName GetItemName() const override;

  bool IsNodeStorageRoot() const override;

 protected:
  NodeStorageRoot(PageGraph* const graph);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeStorageRoot> {
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return node.IsNodeStorageRoot();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeStorageRoot>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_STORAGE_NODE_STORAGE_ROOT_H_
