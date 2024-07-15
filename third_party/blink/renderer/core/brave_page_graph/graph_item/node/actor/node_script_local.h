// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_ACTOR_NODE_SCRIPT_LOCAL_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_ACTOR_NODE_SCRIPT_LOCAL_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave_page_graph {

class NodeScriptLocal : public NodeScript {
 public:
  NodeScriptLocal(GraphItemContext* context,
                  const ScriptId script_id,
                  const ScriptData& script_data);
  ~NodeScriptLocal() override;

  const ScriptData& GetScriptData() const { return script_data_; }

  const String& GetURL() const { return url_; }
  void SetURL(const String url) { url_ = url; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsNodeScriptLocal() const override;

 protected:
  void AddInEdge(const GraphEdge* in_edge) override;

 private:
  const ScriptData script_data_;
  String url_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeScriptLocal> {
  static bool AllowFrom(const brave_page_graph::NodeActor& actor_node) {
    return actor_node.IsNodeScriptLocal();
  }
  static bool AllowFrom(const brave_page_graph::GraphNode& node) {
    return IsA<brave_page_graph::NodeScriptLocal>(
        DynamicTo<brave_page_graph::GraphNode>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeScriptLocal>(
        DynamicTo<brave_page_graph::GraphNode>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_ACTOR_NODE_SCRIPT_LOCAL_H_
