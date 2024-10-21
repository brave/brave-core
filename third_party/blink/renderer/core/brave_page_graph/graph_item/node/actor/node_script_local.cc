// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script_local.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/execute/edge_execute.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

using ::blink::DynamicTo;

namespace brave_page_graph {

namespace {

String GetScriptTypeAsString(const ScriptSource& script_source) {
  if (script_source.is_module) {
    return "module";
  }
  if (script_source.is_eval) {
    return "eval";
  }
  switch (script_source.location_type) {
    case blink::ScriptSourceLocationType::kUnknown:
      return "unknown";
    case blink::ScriptSourceLocationType::kExternalFile:
      return "external file";
    case blink::ScriptSourceLocationType::kInline:
      return "inline";
    case blink::ScriptSourceLocationType::kInlineInsideDocumentWrite:
      return "inline inside document write";
    case blink::ScriptSourceLocationType::kInlineInsideGeneratedElement:
      return "inline inside generated element";
    case blink::ScriptSourceLocationType::kInternal:
      return "internal";
    case blink::ScriptSourceLocationType::kJavascriptUrl:
      return "javascript url";
    case blink::ScriptSourceLocationType::kEvalForScheduledAction:
      return "eval for scheduled action";
    case blink::ScriptSourceLocationType::kInspector:
      return "inspector";
  }
}

}  // namespace

NodeScriptLocal::NodeScriptLocal(GraphItemContext* context,
                                 const ScriptId script_id,
                                 const ScriptData& script_data)
    : NodeScript(context, script_id), script_data_(script_data) {}

NodeScriptLocal::~NodeScriptLocal() = default;

ItemName NodeScriptLocal::GetItemName() const {
  return "script";
}

ItemDesc NodeScriptLocal::GetItemDesc() const {
  StringBuilder ts;
  ts << NodeScript::GetItemDesc();

  if (!script_data_.source.url.IsEmpty()) {
    ts << " [" << script_data_.source.url.GetString() << "]";
  }

  return ts.ReleaseString();
}

void NodeScriptLocal::AddInEdge(const GraphEdge* in_edge) {
  NodeActor::AddInEdge(in_edge);
  if (const EdgeExecute* execute_in_edge = DynamicTo<EdgeExecute>(in_edge)) {
    if (const NodeHTMLElement* element =
            DynamicTo<NodeHTMLElement>(execute_in_edge->GetOutNode());
        element && element->TagName() == "script") {
      const auto& attributes = element->GetAttributes();
      const auto source = attributes.find("src");
      if (source != attributes.end()) {
        url_ = source->value;
      }
    }
  }
}

void NodeScriptLocal::AddGraphMLAttributes(xmlDocPtr doc,
                                           xmlNodePtr parent_node) const {
  NodeScript::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefScriptType)
      ->AddValueNode(doc, parent_node,
                     GetScriptTypeAsString(script_data_.source));
  GraphMLAttrDefForType(kGraphMLAttrDefSource)
      ->AddValueNode(doc, parent_node, script_data_.code);
  GraphMLAttrDefForType(kGraphMLAttrDefURL)
      ->AddValueNode(doc, parent_node, url_);
}

bool NodeScriptLocal::IsNodeScriptLocal() const {
  return true;
}

}  // namespace brave_page_graph
