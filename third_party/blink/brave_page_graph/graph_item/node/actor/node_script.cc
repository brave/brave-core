/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"

#include <sstream>
#include <string>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/execute/edge_execute.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_element.h"

using ::std::string;
using ::std::stringstream;
using ::std::to_string;

using ::blink::DynamicTo;

namespace brave_page_graph {

NodeScript::NodeScript(PageGraph* const graph, const ScriptId script_id,
    const ScriptType type, const std::string& source, const std::string& url) :
      NodeActor(graph),
      script_id_(script_id),
      script_type_(type),
      source_(source),
      url_(url) {}

NodeScript::~NodeScript() {}

ItemName NodeScript::GetItemName() const {
  return "script";
}

ItemDesc NodeScript::GetItemDesc() const {
  stringstream builder;
  builder << NodeActor::GetItemDesc();

  if (!url_.empty()) {
    builder << " [" << url_ << "]";
  }

  return builder.str();
}


void NodeScript::AddInEdge(const Edge* const in_edge) {
  NodeActor::AddInEdge(in_edge);
  if (const EdgeExecute* const execute_in_edge =
          DynamicTo<EdgeExecute>(in_edge)) {
    if (const NodeHTMLElement* const element =
            DynamicTo<NodeHTMLElement>(execute_in_edge->GetOutNode())) {
      if (element->TagName() == "script"
          && element->GetAttributes().count("src") == 1) {
        url_ = element->GetAttributes().at("src");
      }
    }
  }
}

void NodeScript::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  NodeActor::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefScriptIdForNode)
      ->AddValueNode(doc, parent_node, script_id_);
  GraphMLAttrDefForType(kGraphMLAttrDefScriptType)
      ->AddValueNode(doc, parent_node, ScriptTypeToString(script_type_));
  GraphMLAttrDefForType(kGraphMLAttrDefSource)
      ->AddValueNode(doc, parent_node, source_);
  GraphMLAttrDefForType(kGraphMLAttrDefURL)
      ->AddValueNode(doc, parent_node, url_);
}

bool NodeScript::IsNodeScript() const {
  return true;
}

}  // namespace brave_page_graph
