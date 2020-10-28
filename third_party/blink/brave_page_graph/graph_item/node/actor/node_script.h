/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_ACTOR_NODE_SCRIPT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_ACTOR_NODE_SCRIPT_H_

#include <string>
#include <libxml/tree.h>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_actor.h"

namespace brave_page_graph {

class PageGraph;

class NodeScript : public NodeActor {
friend class PageGraph;
 public:
  NodeScript() = delete;
  ~NodeScript() override;

  ScriptId GetScriptId() const { return script_id_; }
  ScriptType GetScriptType() const { return script_type_; }
  const std::string& GetSource() const { return source_; }

  const std::string& GetURL() const { return url_; }
  void SetURL(const std::string url) { url_ = url; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node)
      const override;

  bool IsNodeScript() const override;

 protected:
  NodeScript(PageGraph* const graph, const ScriptId script_id,
    const ScriptType type, const std::string& source,
    const std::string& url = "");

  void AddInEdge(const Edge* const in_edge) override;

 private:
  const ScriptId script_id_;
  const ScriptType script_type_;
  const std::string source_;
  std::string url_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeScript> {
  static bool AllowFrom(const brave_page_graph::NodeActor& actor_node) {
    return actor_node.IsNodeScript();
  }
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return IsA<brave_page_graph::NodeScript>(
        DynamicTo<brave_page_graph::Node>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeScript>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_ACTOR_NODE_SCRIPT_H_
