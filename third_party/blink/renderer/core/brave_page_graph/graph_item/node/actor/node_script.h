/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_ACTOR_NODE_SCRIPT_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_ACTOR_NODE_SCRIPT_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"

namespace brave_page_graph {

class NodeScript : public NodeActor {
 public:
  NodeScript(GraphItemContext* context,
             const ScriptId script_id,
             const ScriptData& script_data);
  ~NodeScript() override;

  ScriptId GetScriptId() const { return script_id_; }
  const ScriptData& GetScriptData() const { return script_data_; }

  const std::string& GetURL() const { return url_; }
  void SetURL(const std::string url) { url_ = url; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsNodeScript() const override;

 protected:
  void AddInEdge(const GraphEdge* in_edge) override;

 private:
  const ScriptId script_id_;
  const ScriptData script_data_;
  std::string url_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeScript> {
  static bool AllowFrom(const brave_page_graph::NodeActor& actor_node) {
    return actor_node.IsNodeScript();
  }
  static bool AllowFrom(const brave_page_graph::GraphNode& node) {
    return IsA<brave_page_graph::NodeScript>(
        DynamicTo<brave_page_graph::GraphNode>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeScript>(
        DynamicTo<brave_page_graph::GraphNode>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_ACTOR_NODE_SCRIPT_H_
