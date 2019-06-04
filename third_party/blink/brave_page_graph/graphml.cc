/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include <string>
#include <vector>
#include "base/logging.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::endl;
using ::std::string;
using ::std::to_string;
using ::std::unique_ptr;
using ::std::vector;

namespace brave_page_graph {

namespace {
  const GraphMLAttr* const attr_name_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "attr name");
  const GraphMLAttr* const attr_value = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "attr value");
  const GraphMLAttr* const before_node_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "before", kGraphMLAttrTypeLong);
  const GraphMLAttr* const call_args = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "args");
  const GraphMLAttr* const edge_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "edge type");
  const GraphMLAttr* const is_style_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "is style", kGraphMLAttrTypeBoolean);
  const GraphMLAttr* const key_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "key");
  const GraphMLAttr* const method_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "method");
  const GraphMLAttr* const tag_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "tag name");
  const GraphMLAttr* const node_id_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "node id", kGraphMLAttrTypeLong);
  const GraphMLAttr* const node_text = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "text");
  const GraphMLAttr* const node_type = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "node type");
  const GraphMLAttr* const parent_node_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "parent", kGraphMLAttrTypeLong);
  const GraphMLAttr* const script_id_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "script id", kGraphMLAttrTypeLong);
  const GraphMLAttr* const script_type = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "script type");
  const GraphMLAttr* const status_type = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "status");
  const GraphMLAttr* const success_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "is success", kGraphMLAttrTypeBoolean);
  const GraphMLAttr* const url_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "url");
  const GraphMLAttr* const request_id_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "request id", kGraphMLAttrTypeLong);
  const GraphMLAttr* const request_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "request type");
  const GraphMLAttr* const resource_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "resource type");
  const GraphMLAttr* const value_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "value");

  const vector<const GraphMLAttr* const> _all_graphml_attrs = {
    attr_name_attr, attr_value, before_node_attr, call_args, edge_type_attr,
    is_style_attr, key_attr, method_attr, tag_attr, node_id_attr,
    node_text, node_type, parent_node_attr, script_id_attr, script_type,
    status_type, success_attr, url_attr, request_id_attr,
    request_type_attr, resource_type_attr, value_attr
  };
}

const vector<const GraphMLAttr* const>& GetGraphMLAttrs() {
  return _all_graphml_attrs;
}

namespace {
  uint32_t graphml_index = 0;
}

GraphMLAttr::GraphMLAttr(const GraphMLAttrForType for_value,
    const string& name) :
      id_(++graphml_index),
      for_(for_value),
      name_(name),
      type_(kGraphMLAttrTypeString) {}

GraphMLAttr::GraphMLAttr(const GraphMLAttrForType for_value, const string& name,
    const GraphMLAttrType type) :
      id_(++graphml_index),
      for_(for_value),
      name_(name),
      type_(type) {}

GraphMLId GraphMLAttr::GetGraphMLId() const {
  return "d" + to_string(id_);
}

GraphMLXML GraphMLAttr::ToDefinition() const {
   return "<key id=\"" + GetGraphMLId() + "\" " +
                "for=\"" + GraphMLForToString(for_) + "\" " +
                "attr.name=\"" + name_ + "\" " +
                "attr.type=\"" + RequestTypeToString(type_) + "\"/>";
}

GraphMLXML GraphMLAttr::ToValue(const char* value) const {
  return ToValue(string(value));
}

GraphMLXML GraphMLAttr::ToValue(const string& value) const {
  LOG_ASSERT(type_ == kGraphMLAttrTypeString);
  return "<data key=\"" + GetGraphMLId() + "\">" +
            "<![CDATA[" + value + "]]>" +
            "</data>";
}

GraphMLXML GraphMLAttr::ToValue(const int value) const {
  LOG_ASSERT(type_ == kGraphMLAttrTypeLong);
  return "<data key=\"" + GetGraphMLId() + "\">" + to_string(value) + "</data>";
}

GraphMLXML GraphMLAttr::ToValue(const bool value) const {
  LOG_ASSERT(type_ == kGraphMLAttrTypeBoolean);
  return "<data key=\"" + GetGraphMLId() + "\">" + to_string(value) + "</data>";
}

GraphMLXML GraphMLAttr::ToValue(const uint64_t value) const {
  LOG_ASSERT(type_ == kGraphMLAttrTypeLong);
  return "<data key=\"" + GetGraphMLId() + "\">" + to_string(value) + "</data>";

}

const GraphMLAttr* GraphMLAttrDefForType(const GraphMLAttrDef type) noexcept {
  switch (type) {
    case kGraphMLAttrDefAttrName:
      return attr_name_attr;
    case kGraphMLAttrDefBeforeNodeId:
      return before_node_attr;
    case kGraphMLAttrDefCallArgs:
      return call_args;
    case kGraphMLAttrDefEdgeType:
      return edge_type_attr;
    case kGraphMLAttrDefIsStyle:
      return is_style_attr;
    case kGraphMLAttrDefKey:
      return key_attr;
    case kGraphMLAttrDefMethodName:
      return method_attr;
    case kGraphMLAttrDefNodeTag:
      return tag_attr;
    case kGraphMLAttrDefNodeId:
      return node_id_attr;
    case kGraphMLAttrDefNodeText:
      return node_text;
    case kGraphMLAttrDefNodeType:
      return node_type;
    case kGraphMLAttrDefParentNodeId:
      return parent_node_attr;
    case kGraphMLAttrDefRequestId:
      return request_id_attr;
    case kGraphMLAttrDefRequestType:
      return request_type_attr;
    case kGraphMLAttrDefResourceType:
      return resource_type_attr;
    case kGraphMLAttrDefScriptId:
      return script_id_attr;
    case kGraphMLAttrDefScriptType:
      return script_type;
    case kGraphMLAttrDefStatus:
      return status_type;
    case kGraphMLAttrDefSuccess:
      return success_attr;
    case kGraphMLAttrDefUrl:
      return url_attr;
    case kGraphMLAttrDefValue:
      return value_attr;
    case kGraphMLAttrDefUnknown:
      return nullptr;
  }
}

}  // namespace brave_page_graph
