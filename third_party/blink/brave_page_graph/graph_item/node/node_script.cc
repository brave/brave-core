/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

NodeScript::NodeScript(PageGraph* const graph, const ScriptId script_id,
    const ScriptType type) :
      NodeActor(graph),
      script_id_(script_id),
      type_(type),
      url_(""),
      is_inline_(true) {}

NodeScript::NodeScript(PageGraph* const graph, const ScriptId script_id,
    const ScriptType type, const std::string& url) :
      NodeActor(graph),
      script_id_(script_id),
      type_(type),
      url_(url),
      is_inline_(true) {}

NodeScript::~NodeScript() {}

ItemName NodeScript::GetItemName() const {
  return "NodeScript#" + to_string(id_);
}

ScriptId NodeScript::GetScriptId() const {
  return script_id_;
}

ScriptType NodeScript::GetScriptType() const {
  return type_;
}

bool NodeScript::IsScript() const {
  return true;
}

bool NodeScript::IsInline() const {
  return is_inline_;
}

string NodeScript::GetUrl() const {
  return url_;
}

GraphMLXMLList NodeScript::GraphMLAttributes() const {
  GraphMLXMLList attrs;
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("script"));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefScriptId)
      ->ToValue(script_id_));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefScriptType)
      ->ToValue(ScriptTypeToString(type_)));

  if (IsInline() == false) {
    attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefUrl)
      ->ToValue(GetUrl()));
  }

  return attrs;
}

ItemDesc NodeScript::GetDescBody() const {
  return GetItemName() +
    " [ScriptId:" + to_string(script_id_) +
    ", Type:"  + ScriptTypeToString(type_) + "]"; 
}

}  // namespace brave_page_graph
