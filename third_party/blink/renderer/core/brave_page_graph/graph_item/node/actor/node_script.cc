// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

using ::blink::DynamicTo;

namespace brave_page_graph {

NodeScript::NodeScript(GraphItemContext* context, const ScriptId script_id)
    : NodeActor(context), script_id_(script_id) {}

ItemDesc NodeScript::GetItemDesc() const {
  return blink::StrCat({NodeActor::GetItemDesc(), " [ script_id: ",
                        blink::String::Number(script_id_), "]"});
}

void NodeScript::AddGraphMLAttributes(xmlDocPtr doc,
                                      xmlNodePtr parent_node) const {
  NodeActor::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefNodeScriptId)
      ->AddValueNode(doc, parent_node, script_id_);
}

bool NodeScript::IsNodeScript() const {
  return true;
}

}  // namespace brave_page_graph
