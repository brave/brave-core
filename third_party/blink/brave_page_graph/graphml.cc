/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graphml.h"

#include <map>
#include <string>
#include <vector>

#include <libxml/entities.h>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/logging.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::chrono::milliseconds;
using ::std::endl;
using ::std::map;
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
  const GraphMLAttr* const binding_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "binding");
  const GraphMLAttr* const binding_event_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "binding event");
  const GraphMLAttr* const binding_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "binding type");
  const GraphMLAttr* const block_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "block type");
  const GraphMLAttr* const call_args_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "args");
  const GraphMLAttr* const edge_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "edge type");
  const GraphMLAttr* const event_listener_id_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "event listener id", kGraphMLAttrTypeLong);
  const GraphMLAttr* const frame_id_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "frame id");
  const GraphMLAttr* const host_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "host");
  const GraphMLAttr* const incognito_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "incognito");
  const GraphMLAttr* const is_deleted_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "is deleted", kGraphMLAttrTypeBoolean);
  const GraphMLAttr* const is_style_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "is style", kGraphMLAttrTypeBoolean);
  const GraphMLAttr* const key_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "key");
  const GraphMLAttr* const method_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "method");
  const GraphMLAttr* const node_id_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "node id", kGraphMLAttrTypeLong);
  const GraphMLAttr* const node_text_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "text");
  const GraphMLAttr* const node_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "node type");
  const GraphMLAttr* const page_graph_edge_id_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "id", kGraphMLAttrTypeLong);
  const GraphMLAttr* const page_graph_node_id_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "id", kGraphMLAttrTypeLong);
  const GraphMLAttr* const page_graph_edge_time_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "timestamp", kGraphMLAttrTypeLong);
  const GraphMLAttr* const page_graph_node_time_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "timestamp", kGraphMLAttrTypeLong);
  const GraphMLAttr* const parent_node_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "parent", kGraphMLAttrTypeLong);
  const GraphMLAttr* const primary_pattern_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "primary pattern");
  const GraphMLAttr* const request_id_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "request id", kGraphMLAttrTypeLong);
  const GraphMLAttr* const request_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "request type");
  const GraphMLAttr* const resource_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "resource type");
  const GraphMLAttr* const response_hash_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "response hash");
  const GraphMLAttr* const rule_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "rule");
  const GraphMLAttr* const script_id_for_edge_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "script id", kGraphMLAttrTypeLong);
  const GraphMLAttr* const script_id_for_node_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "script id", kGraphMLAttrTypeLong);
  const GraphMLAttr* const script_position_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "script position", kGraphMLAttrTypeLong);
  const GraphMLAttr* const script_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "script type");
  const GraphMLAttr* const secondary_pattern_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "secondary pattern");
  const GraphMLAttr* const source_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "source");
  const GraphMLAttr* const status_type_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "status");
  const GraphMLAttr* const success_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "is success", kGraphMLAttrTypeBoolean);
  const GraphMLAttr* const tag_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "tag name");
  const GraphMLAttr* const url_attr = new GraphMLAttr(
    kGraphMLAttrForTypeNode, "url");
  const GraphMLAttr* const value_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "value");
  const GraphMLAttr* const size_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "size");
  const GraphMLAttr* const headers_attr = new GraphMLAttr(
    kGraphMLAttrForTypeEdge, "headers");

  const vector<const GraphMLAttr* const> _all_graphml_attrs = {
    attr_name_attr, attr_value, before_node_attr, binding_attr,
    binding_event_attr, binding_type_attr, block_type_attr, call_args_attr,
    edge_type_attr, event_listener_id_attr, frame_id_attr, host_attr,
    incognito_attr, is_deleted_attr, is_style_attr, key_attr, method_attr,
    node_id_attr, node_text_attr, node_type_attr, page_graph_edge_id_attr,
    page_graph_node_id_attr, page_graph_edge_time_attr,
    page_graph_node_time_attr, parent_node_attr, primary_pattern_attr,
    request_id_attr, request_type_attr, resource_type_attr, response_hash_attr,
    rule_attr, script_id_for_edge_attr, script_id_for_node_attr,
    script_position_attr, script_type_attr, secondary_pattern_attr, source_attr,
    status_type_attr, success_attr, tag_attr, url_attr, value_attr,
    size_attr, headers_attr
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

void GraphMLAttr::AddDefinitionNode(xmlNodePtr parent_node) const {
  xmlNodePtr new_node = xmlNewChild(parent_node, NULL, BAD_CAST "key", NULL);
  xmlSetProp(new_node, BAD_CAST "id", BAD_CAST GetGraphMLId().c_str());
  xmlSetProp(new_node, BAD_CAST "for",
      BAD_CAST GraphMLForTypeToString(for_).c_str());
  xmlSetProp(new_node, BAD_CAST "attr.name", BAD_CAST name_.c_str());
  xmlSetProp(new_node, BAD_CAST "attr.type",
      BAD_CAST GraphMLAttrTypeToString(type_).c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc, xmlNodePtr parent_node,
    const char* value) const {
  AddValueNode(doc, parent_node, string(value));
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc, xmlNodePtr parent_node,
    const string& value) const {
  PG_LOG_ASSERT(type_ == kGraphMLAttrTypeString);
  xmlChar* encoded_content = xmlEncodeEntitiesReentrant(doc,
      BAD_CAST value.c_str());
  xmlNodePtr new_node = xmlNewChild(parent_node, NULL,
      BAD_CAST "data", encoded_content);
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
  xmlFree(encoded_content);
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc, xmlNodePtr parent_node,
    const int value) const {
  PG_LOG_ASSERT(type_ == kGraphMLAttrTypeLong);
  xmlNodePtr new_node = xmlNewTextChild(parent_node, NULL, BAD_CAST "data",
      BAD_CAST to_string(value).c_str());
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc, xmlNodePtr parent_node,
    const bool value) const {
  PG_LOG_ASSERT(type_ == kGraphMLAttrTypeBoolean);
  xmlNodePtr new_node = xmlNewTextChild(parent_node, NULL, BAD_CAST "data",
      BAD_CAST (value ? "true" : "false"));
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc, xmlNodePtr parent_node,
    const uint64_t value) const {
  PG_LOG_ASSERT(type_ == kGraphMLAttrTypeLong);
  xmlNodePtr new_node = xmlNewTextChild(parent_node, NULL, BAD_CAST "data",
      BAD_CAST to_string(value).c_str());
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc, xmlNodePtr parent_node,
    const double value) const {
  PG_LOG_ASSERT(type_ == kGraphMLAttrTypeDouble);
  xmlNodePtr new_node = xmlNewTextChild(parent_node, NULL, BAD_CAST "data",
      BAD_CAST to_string(value).c_str());
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc, xmlNodePtr parent_node,
    const milliseconds value) const {
  PG_LOG_ASSERT(type_ == kGraphMLAttrTypeLong);
  xmlNodePtr new_node = xmlNewTextChild(parent_node, NULL, BAD_CAST "data",
      BAD_CAST to_string(value.count()).c_str());
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

const GraphMLAttr* GraphMLAttrDefForType(const GraphMLAttrDef type) noexcept {
  switch (type) {
    case kGraphMLAttrDefAttrName:
      return attr_name_attr;
    case kGraphMLAttrDefBeforeNodeId:
      return before_node_attr;
    case kGraphMLAttrDefBinding:
      return binding_attr;
    case kGraphMLAttrDefBindingEvent:
      return binding_event_attr;
    case kGraphMLAttrDefBindingType:
      return binding_type_attr;
    case kGraphMLAttrDefBlockType:
      return block_type_attr;
    case kGraphMLAttrDefCallArgs:
      return call_args_attr;
    case kGraphMLAttrDefEdgeType:
      return edge_type_attr;
    case kGraphMLAttrDefEventListenerId:
      return event_listener_id_attr;
    case kGraphMLAttrDefFrameId:
      return frame_id_attr;
    case kGraphMLAttrDefHost:
      return host_attr;
    case kGraphMLAttrDefIncognito:
      return incognito_attr;
    case kGraphMLAttrDefIsDeleted:
      return is_deleted_attr;
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
      return node_text_attr;
    case kGraphMLAttrDefNodeType:
      return node_type_attr;
    case kGraphMLAttrDefPageGraphEdgeId:
      return page_graph_edge_id_attr;
    case kGraphMLAttrDefPageGraphNodeId:
      return page_graph_node_id_attr;
    case kGraphMLAttrDefPageGraphEdgeTimestamp:
      return page_graph_edge_time_attr;
    case kGraphMLAttrDefPageGraphNodeTimestamp:
      return page_graph_node_time_attr;
    case kGraphMLAttrDefParentNodeId:
      return parent_node_attr;
    case kGraphMLAttrDefPrimaryPattern:
      return primary_pattern_attr;
    case kGraphMLAttrDefRequestId:
      return request_id_attr;
    case kGraphMLAttrDefRequestType:
      return request_type_attr;
    case kGraphMLAttrDefResourceType:
      return resource_type_attr;
    case kGraphMLAttrDefResponseHash:
      return response_hash_attr;
    case kGraphMLAttrDefRule:
      return rule_attr;
    case kGraphMLAttrDefScriptIdForEdge:
      return script_id_for_edge_attr;
    case kGraphMLAttrDefScriptIdForNode:
      return script_id_for_node_attr;
    case kGraphMLAttrDefScriptPosition:
      return script_position_attr;
    case kGraphMLAttrDefScriptType:
      return script_type_attr;
    case kGraphMLAttrDefSecondaryPattern:
      return secondary_pattern_attr;
    case kGraphMLAttrDefSource:
      return source_attr;
    case kGraphMLAttrDefStatus:
      return status_type_attr;
    case kGraphMLAttrDefSuccess:
      return success_attr;
    case kGraphMLAttrDefURL:
      return url_attr;
    case kGraphMLAttrDefValue:
      return value_attr;
    case kGraphMLAttrDefUnknown:
      return nullptr;
    case kGraphMLAttrDefSize:
      return size_attr;
    case kGraphMLAttrDefHeaders:
      return headers_attr;
  }
}

}  // namespace brave_page_graph
