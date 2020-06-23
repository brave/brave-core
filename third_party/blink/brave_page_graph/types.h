/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_TYPES_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

class GURL;

namespace blink {
using DOMNodeId = int;
class KURL;
enum class ResourceType : uint8_t;
class ScriptSourceCode;
class WebString;
}  // namespace blink

namespace v8 {
class Isolate;
}

namespace WTF {
class String;
}  // namespace WTF

namespace brave_page_graph {

class Element;
class Edge;
class EdgeRequestResponse;
class EdgeRequestStart;
class GraphItem;
class Node;
class NodeHTML;

typedef std::string ItemDesc;
typedef std::string ItemName;
typedef std::string GraphMLId;

typedef enum {
  kGraphMLAttrDefAttrName = 0,
  kGraphMLAttrDefBeforeNodeId,
  kGraphMLAttrDefBlockType,
  kGraphMLAttrDefCallArgs,
  kGraphMLAttrDefEdgeType,
  kGraphMLAttrDefEventListenerId,
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
  kGraphMLAttrDefRequestType,
  kGraphMLAttrDefResourceType,
  kGraphMLAttrDefResponseHash,
  kGraphMLAttrDefRule,
  kGraphMLAttrDefScriptId,
  kGraphMLAttrDefScriptType,
  kGraphMLAttrDefSecondaryPattern,
  kGraphMLAttrDefSource,
  kGraphMLAttrDefStatus,
  kGraphMLAttrDefSuccess,
  kGraphMLAttrDefURL,
  kGraphMLAttrDefValue,
  kGraphMLAttrDefUnknown,
} GraphMLAttrDef;

typedef enum {
  kGraphMLAttrTypeString = 0,
  kGraphMLAttrTypeBoolean,
  kGraphMLAttrTypeInt,
  kGraphMLAttrTypeLong,
  kGraphMLAttrTypeFloat,
  kGraphMLAttrTypeDouble,
  kGraphMLAttrTypeUnknown
} GraphMLAttrType;
std::string GraphMLAttrTypeToString(const GraphMLAttrType type) noexcept;

typedef enum {
  kGraphMLAttrForTypeNode = 0,
  kGraphMLAttrForTypeEdge,
  kGraphMLAttrForTypeUnknown
} GraphMLAttrForType;
std::string GraphMLForTypeToString(const GraphMLAttrForType type) noexcept;

typedef enum {
  kRequestTypeAJAX = 0,
  kRequestTypeAudio,
  kRequestTypeCSS,
  kRequestTypeDocument,
  kRequestTypeFont,
  kRequestTypeImage,
  kRequestTypeScriptClassic,
  kRequestTypeScriptModule,
  kRequestTypeSVG,
  kRequestTypeVideo,
  kRequestTypeUnknown
} RequestType;
std::string RequestTypeToString(const RequestType type) noexcept;

std::string ResourceTypeToString(const blink::ResourceType type) noexcept;

typedef enum {
  kScriptTypeClassic = 0,
  kScriptTypeModule,
  kScriptTypeExtension,
  kExternalFile,
  kInline,
  kInlineInsideDocumentWrite,
  kInlineInsideGeneratedElement,
  kInternal,
  kJavascriptUrl,
  kEvalForScheduledAction,
  kInspector,
  kScriptTypeUnknown
} ScriptType;
std::string ScriptTypeToString(const ScriptType type) noexcept;

typedef enum {
  kElementTypeDefault = 0,
  kElementTypeFrameOwner,
} ElementType;

typedef enum {
  kRequestStatusStart = 0,
  kRequestStatusComplete,
  kRequestStatusError,
  kRequestStatusBlocked
} RequestStatus;
std::string RequestStatusToString(const RequestStatus status) noexcept;

typedef enum {
  kStorageLocationCookie = 0,
  kStorageLocationLocalStorage,
  kStorageLocationSessionStorage
} StorageLocation;
std::string StorageLocationToString(const StorageLocation location) noexcept;

typedef enum {
  kJSBuiltInDateNow = 0,
  kJSBuiltInDateConstructor,
  kJSBuiltInDateParse,
  kJSBuiltInDateUTC,
  kJSBuiltInDatePrototypeSetDate,
  kJSBuiltInDatePrototypeSetFullYear,
  kJSBuiltInDatePrototypeSetHours,
  kJSBuiltInDatePrototypeSetMilliseconds,
  kJSBuiltInDatePrototypeSetMinutes,
  kJSBuiltInDatePrototypeSetMonth,
  kJSBuiltInDatePrototypeSetSeconds,
  kJSBuiltInDatePrototypeSetTime,
  kJSBuiltInDatePrototypeSetUTCDate,
  kJSBuiltInDatePrototypeSetUTCFullYear,
  kJSBuiltInDatePrototypeSetUTCHours,
  kJSBuiltInDatePrototypeSetUTCMilliseconds,
  kJSBuiltInDatePrototypeSetUTCMinutes,
  kJSBuiltInDatePrototypeSetUTCMonth,
  kJSBuiltInDatePrototypeSetUTCSeconds,
  kJSBuiltInDatePrototypeToDateString,
  kJSBuiltInDatePrototypeToISOString,
  kJSBuiltInDatePrototypeToString,
  kJSBuiltInDatePrototypeToTimeString,
  kJSBuiltInDatePrototypeToLocaleDateString,
  kJSBuiltInDatePrototypeToLocalString,
  kJSBuiltInDatePrototypeToLocalTimeString,
  kJSBuiltInDatePrototypeToUTCString,
  kJSBuiltInDatePrototypeGetYear,
  kJSBuiltInDatePrototypeSetYear,
  kJSBuiltInDatePrototypeToJSON,
  kJSBuiltInJSONParse,
  kJSBuiltInJSONStringify,
} JSBuiltIn;
JSBuiltIn JSBuiltInFromString(const std::string& built_in_name) noexcept;
const std::string& JSBuiltInToSting(const JSBuiltIn built_in_name) noexcept;

typedef enum {
  kWebAPIDocumentReferrer = 0,
  kWebAPILocationAncestorOrigins,
  kWebAPILocationAssign,
  kWebAPILocationHash,
  kWebAPILocationHost,
  kWebAPILocationHostname,
  kWebAPILocationHref,
  kWebAPILocationOrigin,
  kWebAPILocationPathname,
  kWebAPILocationPort,
  kWebAPILocationProtocol,
  kWebAPILocationReload,
  kWebAPILocationReplace,
  kWebAPILocationSearch,
} WebAPI;
WebAPI WebAPIFromString(const std::string& built_in_name) noexcept;
const std::string& WebAPIToString(const WebAPI built_in_name) noexcept;

typedef unsigned SourceCodeHash;
typedef unsigned UrlHash;
typedef int ScriptId;
typedef int EventListenerId;
typedef uint64_t PageGraphId;
typedef std::string MethodName;
typedef std::string RequestURL;
typedef uint64_t InspectorId;

struct FingerprintingRule {
  const std::string& primary_pattern;
  const std::string& secondary_pattern;
  const std::string& source;
  const bool incognito;

  FingerprintingRule(const std::string& primary_pattern,
                     const std::string& secondary_pattern,
                     const std::string& source, const bool incognito);

  bool operator==(const FingerprintingRule& other) const;
  bool operator<(const FingerprintingRule& other) const;
  bool operator>(const FingerprintingRule& other) const;

  std::string ToString() const;
};

struct EventListener {
  const std::string& event_type;
  const ScriptId listener_script_id;

  EventListener(const std::string& event_type,
      ScriptId const listener_script_id) :
        event_type(event_type),
        listener_script_id(listener_script_id) {}
};

typedef std::vector<const Edge*> EdgeList;
typedef std::vector<Node*> NodeList;
typedef std::vector<std::unique_ptr<const Edge> > EdgeUniquePtrList;
typedef std::vector<std::unique_ptr<Node> > NodeUniquePtrList;
typedef std::vector<const GraphItem*> GraphItemList;
typedef std::vector<NodeHTML*> HTMLNodeList;
typedef std::vector<blink::DOMNodeId> DOMNodeIdList;
typedef std::vector<ScriptId> ScriptIdList;
typedef std::map<const std::string, const std::string> AttributeMap;
typedef std::map<EventListenerId, const EventListener> EventListenerMap;

typedef std::map<SourceCodeHash, ScriptId> HashToScriptIdMap;
typedef std::map<ScriptId, SourceCodeHash> ScriptIdToHashMap;
typedef std::map<SourceCodeHash, UrlHash> SourceToUrlMap;
typedef std::map<UrlHash, SourceCodeHash> UrlToSourceMap;

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_TYPES_H_
