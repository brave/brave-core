/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_REMOTE_FRAME_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_REMOTE_FRAME_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

namespace brave_page_graph {

class NodeRemoteFrame : public Node {
friend class PageGraph;
 public:
  NodeRemoteFrame() = delete;
  ~NodeRemoteFrame() override;

  RequestURL GetURL() const { return url_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsNodeRemoteFrame() const override;

 protected:
  NodeRemoteFrame(PageGraph* const graph, const RequestURL url);

 private:
  const RequestURL url_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeRemoteFrame> {
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return node.IsNodeRemoteFrame();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeRemoteFrame>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_REMOTE_FRAME_H_
