/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

#include <libxml/entities.h>
#include <libxml/tree.h>

#include <string>
#include <string_view>

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/libxml_utils.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"

namespace brave_page_graph {

namespace {
uint32_t graphml_index = 0;
}

GraphMLAttr::GraphMLAttr(const GraphMLAttrForType for_value,
                         const String& name,
                         const GraphMLAttrType type)
    : id_(++graphml_index), for_(for_value), name_(name), type_(type) {}

GraphMLId GraphMLAttr::GetGraphMLId() const {
  return "d" + base::NumberToString(id_);
}

void GraphMLAttr::AddDefinitionNode(xmlNodePtr parent_node) const {
  xmlNodePtr new_node =
      xmlNewChild(parent_node, nullptr, BAD_CAST "key", nullptr);
  xmlSetProp(new_node, BAD_CAST "id", BAD_CAST GetGraphMLId().c_str());
  xmlSetProp(new_node, BAD_CAST "for",
             BAD_CAST GraphMLForTypeToString(for_).c_str());
  xmlSetProp(new_node, BAD_CAST "attr.name", XmlUtf8String(name_).get());
  xmlSetProp(new_node, BAD_CAST "attr.type",
             BAD_CAST GraphMLAttrTypeToString(type_).c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc,
                               xmlNodePtr parent_node,
                               std::string_view value) const {
  AddValueNodeXmlChar(doc, parent_node, XmlUtf8String(value).get());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc,
                               xmlNodePtr parent_node,
                               const String& value) const {
  AddValueNodeXmlChar(doc, parent_node, XmlUtf8String(value).get());
}

void GraphMLAttr::AddValueNodeXmlChar(xmlDocPtr doc,
                                      xmlNodePtr parent_node,
                                      const xmlChar* value) const {
  CHECK(type_ == kGraphMLAttrTypeString);
  xmlChar* encoded_content = xmlEncodeEntitiesReentrant(doc, value);
  xmlNodePtr new_node =
      xmlNewChild(parent_node, nullptr, BAD_CAST "data", encoded_content);
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
  xmlFree(encoded_content);
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc,
                               xmlNodePtr parent_node,
                               const int value) const {
  CHECK(type_ == kGraphMLAttrTypeInt);
  xmlNodePtr new_node =
      xmlNewTextChild(parent_node, nullptr, BAD_CAST "data",
                      BAD_CAST base::NumberToString(value).c_str());
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc,
                               xmlNodePtr parent_node,
                               const bool value) const {
  CHECK(type_ == kGraphMLAttrTypeBoolean);
  xmlNodePtr new_node = xmlNewTextChild(parent_node, nullptr, BAD_CAST "data",
                                        BAD_CAST(value ? "true" : "false"));
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc,
                               xmlNodePtr parent_node,
                               const int64_t value) const {
  CHECK(type_ == kGraphMLAttrTypeString);
  xmlNodePtr new_node =
      xmlNewTextChild(parent_node, nullptr, BAD_CAST "data",
                      BAD_CAST base::NumberToString(value).c_str());
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc,
                               xmlNodePtr parent_node,
                               const uint64_t value) const {
  CHECK(type_ == kGraphMLAttrTypeString);
  xmlNodePtr new_node =
      xmlNewTextChild(parent_node, nullptr, BAD_CAST "data",
                      BAD_CAST base::NumberToString(value).c_str());
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc,
                               xmlNodePtr parent_node,
                               const double value) const {
  CHECK(type_ == kGraphMLAttrTypeDouble);
  xmlNodePtr new_node =
      xmlNewTextChild(parent_node, nullptr, BAD_CAST "data",
                      BAD_CAST base::NumberToString(value).c_str());
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

void GraphMLAttr::AddValueNode(xmlDocPtr doc,
                               xmlNodePtr parent_node,
                               const base::TimeDelta value) const {
  CHECK(type_ == kGraphMLAttrTypeInt);
  xmlNodePtr new_node = xmlNewTextChild(
      parent_node, nullptr, BAD_CAST "data",
      BAD_CAST base::NumberToString(value.InMilliseconds()).c_str());
  xmlSetProp(new_node, BAD_CAST "key", BAD_CAST GetGraphMLId().c_str());
}

const GraphMLAttrs& GetGraphMLAttrs() {
  static base::NoDestructor<GraphMLAttrs> attrs({
      {kGraphMLAttrDefAttrName,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "attr name")},
      {kGraphMLAttrDefBeforeNodeId,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "before", kGraphMLAttrTypeInt)},
      {kGraphMLAttrDefBinding,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "binding")},
      {kGraphMLAttrDefBindingEvent,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "binding event")},
      {kGraphMLAttrDefBindingType,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "binding type")},
      {kGraphMLAttrDefBlockType,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "block type")},
      {kGraphMLAttrDefCallArgs,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "args")},
      {kGraphMLAttrDefEdgeType,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "edge type")},
      {kGraphMLAttrDefEventListenerId,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "event listener id",
                       kGraphMLAttrTypeInt)},
      {kGraphMLAttrDefFrameId,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "frame id")},
      {kGraphMLAttrDefHost, new GraphMLAttr(kGraphMLAttrForTypeNode, "host")},
      {kGraphMLAttrDefIncognito,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "incognito")},
      {kGraphMLAttrDefIsDeleted,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "is deleted",
                       kGraphMLAttrTypeBoolean)},
      {kGraphMLAttrDefIsStyle,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "is style",
                       kGraphMLAttrTypeBoolean)},
      {kGraphMLAttrDefKey, new GraphMLAttr(kGraphMLAttrForTypeEdge, "key")},
      {kGraphMLAttrDefMethodName,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "method")},
      {kGraphMLAttrDefNodeId, new GraphMLAttr(kGraphMLAttrForTypeNode,
                                              "node id", kGraphMLAttrTypeInt)},
      {kGraphMLAttrDefNodeText,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "text")},
      {kGraphMLAttrDefNodeType,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "node type")},
      {kGraphMLAttrDefPageGraphEdgeId,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "id")},
      {kGraphMLAttrDefPageGraphNodeId,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "id")},
      {kGraphMLAttrDefPageGraphEdgeTimestamp,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "timestamp")},
      {kGraphMLAttrDefPageGraphNodeTimestamp,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "timestamp")},
      {kGraphMLAttrDefParentNodeId,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "parent", kGraphMLAttrTypeInt)},
      {kGraphMLAttrDefPrimaryPattern,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "primary pattern")},
      {kGraphMLAttrDefRequestId,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "request id")},
      {kGraphMLAttrDefResourceType,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "resource type")},
      {kGraphMLAttrDefResponseHash,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "response hash")},
      {kGraphMLAttrDefRule, new GraphMLAttr(kGraphMLAttrForTypeNode, "rule")},
      {kGraphMLAttrDefScriptIdForEdge,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "script id",
                       kGraphMLAttrTypeInt)},
      {kGraphMLAttrDefScriptIdForNode,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "script id",
                       kGraphMLAttrTypeInt)},
      {kGraphMLAttrDefScriptPosition,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "script position",
                       kGraphMLAttrTypeInt)},
      {kGraphMLAttrDefScriptType,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "script type")},
      {kGraphMLAttrDefSecondaryPattern,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "secondary pattern")},
      {kGraphMLAttrDefSource,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "source")},
      {kGraphMLAttrDefStatus,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "status")},
      {kGraphMLAttrDefSuccess,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "is success",
                       kGraphMLAttrTypeBoolean)},
      {kGraphMLAttrDefNodeTag,
       new GraphMLAttr(kGraphMLAttrForTypeNode, "tag name")},
      {kGraphMLAttrDefURL, new GraphMLAttr(kGraphMLAttrForTypeNode, "url")},
      {kGraphMLAttrDefValue, new GraphMLAttr(kGraphMLAttrForTypeEdge, "value")},
      {kGraphMLAttrDefSize, new GraphMLAttr(kGraphMLAttrForTypeEdge, "size")},
      {kGraphMLAttrDefHeaders,
       new GraphMLAttr(kGraphMLAttrForTypeEdge, "headers")},
  });
  return *attrs;
}

const GraphMLAttr* GraphMLAttrDefForType(const GraphMLAttrDef type) {
  const auto& attrs = GetGraphMLAttrs();
  auto it = attrs.find(type);
  DCHECK(it != attrs.end());
  return it->second;
}

}  // namespace brave_page_graph
