/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_TYPES_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_TYPES_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_probe_types.h"
#include "third_party/blink/renderer/bindings/core/v8/script_source_location_type.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/graphics/dom_node_id.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave_page_graph {

class GraphItem;
class GraphEdge;
class GraphNode;
class NodeHTML;

using ItemDesc = std::string;
using ItemName = std::string;
using GraphMLId = std::string;

using Binding = const char*;
using BindingType = const char*;
using BindingEvent = const char*;

using ScriptId = int;
using ScriptPosition = int;
using EventListenerId = int;
using GraphItemId = uint64_t;
using MethodName = std::string;
using RequestURL = std::string;
using InspectorId = uint64_t;

using GraphItemUniquePtrList = std::vector<std::unique_ptr<GraphItem>>;
using EdgeList = std::vector<raw_ptr<const GraphEdge>>;
using NodeList = std::vector<raw_ptr<GraphNode>>;
using HTMLNodeList = std::vector<NodeHTML*>;
using AttributeMap = std::map<const std::string, const std::string>;

struct CORE_EXPORT FingerprintingRule {
  FingerprintingRule(const std::string& primary_pattern,
                     const std::string& secondary_pattern,
                     const std::string& source,
                     const bool incognito);

  bool operator==(const FingerprintingRule& other) const;
  bool operator<(const FingerprintingRule& other) const;
  bool operator>(const FingerprintingRule& other) const;

  std::string ToString() const;

  const std::string& primary_pattern;
  const std::string& secondary_pattern;
  const std::string& source;
  const bool incognito;
};

struct ScriptSource {
  blink::DOMNodeId dom_node_id = blink::kInvalidDOMNodeId;
  ScriptId parent_script_id = 0;
  blink::KURL url;
  String function_name;
  blink::ScriptSourceLocationType location_type =
      blink::ScriptSourceLocationType::kUnknown;
  bool is_module = false;
  bool is_eval = false;

  bool operator==(const ScriptSource& rhs) const;
};

struct ScriptData {
  String code;
  ScriptSource source;

  bool operator==(const ScriptData& rhs) const;
};

CORE_EXPORT const char* BindingTypeToString(
    blink::PageGraphBindingType binding_type);
CORE_EXPORT const char* BindingEventToString(
    blink::PageGraphBindingEvent binding_event);

enum GraphMLAttrDef {
  kGraphMLAttrDefAttrName = 0,
  kGraphMLAttrDefBeforeNodeId,
  kGraphMLAttrDefBinding,
  kGraphMLAttrDefBindingEvent,
  kGraphMLAttrDefBindingType,
  kGraphMLAttrDefBlockType,
  kGraphMLAttrDefCallArgs,
  kGraphMLAttrDefEdgeType,
  kGraphMLAttrDefEventListenerId,
  kGraphMLAttrDefFrameId,
  kGraphMLAttrDefHost,
  kGraphMLAttrDefIncognito,
  kGraphMLAttrDefIsDeleted,
  kGraphMLAttrDefIsStyle,
  kGraphMLAttrDefKey,
  kGraphMLAttrDefMethodName,
  kGraphMLAttrDefNodeId,
  kGraphMLAttrDefNodeTag,
  kGraphMLAttrDefNodeText,
  kGraphMLAttrDefNodeType,
  kGraphMLAttrDefPageGraphEdgeId,
  kGraphMLAttrDefPageGraphNodeId,
  kGraphMLAttrDefPageGraphEdgeTimestamp,
  kGraphMLAttrDefPageGraphNodeTimestamp,
  kGraphMLAttrDefParentNodeId,
  kGraphMLAttrDefPrimaryPattern,
  kGraphMLAttrDefRequestId,
  kGraphMLAttrDefResourceType,
  kGraphMLAttrDefResponseHash,
  kGraphMLAttrDefRule,
  kGraphMLAttrDefScriptIdForEdge,
  kGraphMLAttrDefScriptIdForNode,
  kGraphMLAttrDefScriptPosition,
  kGraphMLAttrDefScriptType,
  kGraphMLAttrDefSecondaryPattern,
  kGraphMLAttrDefSource,
  kGraphMLAttrDefStatus,
  kGraphMLAttrDefSuccess,
  kGraphMLAttrDefURL,
  kGraphMLAttrDefValue,
  kGraphMLAttrDefUnknown,
  kGraphMLAttrDefSize,
  kGraphMLAttrDefHeaders,
};

enum GraphMLAttrType {
  kGraphMLAttrTypeString = 0,
  kGraphMLAttrTypeBoolean,
  kGraphMLAttrTypeInt,
  kGraphMLAttrTypeFloat,
  kGraphMLAttrTypeDouble,
  kGraphMLAttrTypeUnknown
};
CORE_EXPORT std::string GraphMLAttrTypeToString(const GraphMLAttrType type);

enum GraphMLAttrForType {
  kGraphMLAttrForTypeNode = 0,
  kGraphMLAttrForTypeEdge,
  kGraphMLAttrForTypeUnknown
};
CORE_EXPORT std::string GraphMLForTypeToString(const GraphMLAttrForType type);

enum RequestStatus {
  kRequestStatusStart = 0,
  kRequestStatusComplete,
  kRequestStatusError,
  kRequestStatusBlocked
};
CORE_EXPORT std::string RequestStatusToString(const RequestStatus status);

enum class StorageLocation {
  kCookie = 0,
  kLocalStorage,
  kSessionStorage,
};
CORE_EXPORT std::string StorageLocationToString(const StorageLocation location);

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_TYPES_H_
