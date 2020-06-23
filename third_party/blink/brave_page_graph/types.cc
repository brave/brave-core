/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/types.h"
#include <map>
#include <string>
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "brave/third_party/blink/brave_page_graph/logging.h"

using ::std::map;
using ::std::string;

namespace brave_page_graph {

string GraphMLAttrTypeToString(const GraphMLAttrType type) noexcept {
  switch (type) {
    case kGraphMLAttrTypeString:
      return "string";
    case kGraphMLAttrTypeBoolean:
      return "boolean";
    case kGraphMLAttrTypeInt:
      return "int";
    case kGraphMLAttrTypeLong:
      return "long";
    case kGraphMLAttrTypeFloat:
      return "float";
    case kGraphMLAttrTypeDouble:
      return "double";
    case kGraphMLAttrTypeUnknown:
    default:
      return "unknown";
  }
}

string GraphMLForTypeToString(const GraphMLAttrForType type) noexcept {
  switch (type) {
    case kGraphMLAttrForTypeNode:
      return "node";
    case kGraphMLAttrForTypeEdge:
      return "edge";
    case kGraphMLAttrForTypeUnknown:
    default:
      return "unknown";
  }
}

string RequestTypeToString(const RequestType type) noexcept {
  switch (type) {
    case kRequestTypeAJAX:
      return "AJAX";
    case kRequestTypeImage:
      return "Image";
    case kRequestTypeScriptClassic:
      return "ScriptClassic";
    case kRequestTypeScriptModule:
      return "ScriptModule";
    case kRequestTypeCSS:
      return "CSS";
    case kRequestTypeVideo:
      return "Video";
    case kRequestTypeAudio:
      return "Audio";
    case kRequestTypeSVG:
      return "SVG";
    case kRequestTypeFont:
      return "Font";
    case kRequestTypeDocument:
      return "Document";
    case kRequestTypeUnknown:
    default:
      return "Unknown";
  }
}

string ResourceTypeToString(const blink::ResourceType type) noexcept {
  switch (type) {
    case blink::ResourceType::kImage:
      return "image";
    case blink::ResourceType::kCSSStyleSheet:
      return "css";
    case blink::ResourceType::kScript:
      return "script";
    case blink::ResourceType::kFont:
      return "font";
    case blink::ResourceType::kRaw:
      return "raw";
    case blink::ResourceType::kSVGDocument:
      return "svg";
    case blink::ResourceType::kXSLStyleSheet:
      return "XSL style sheet";
    case blink::ResourceType::kLinkPrefetch:
      return "link prefetch";
    case blink::ResourceType::kTextTrack:
      return "text track";
    case blink::ResourceType::kImportResource:
      return "import resource";
    case blink::ResourceType::kAudio:
      return "audio";
    case blink::ResourceType::kVideo:
      return "video";
    case blink::ResourceType::kManifest:
      return "manifest";
    default:
      return "unknown";
  }
}

string ScriptTypeToString(const ScriptType type) noexcept {
  switch (type) {
    case kScriptTypeClassic:
      return "classic";
    case kScriptTypeModule:
      return "module";
    case kScriptTypeExtension:
      return "extension";
    case kExternalFile:
      return "external file";
    case kInline:
      return "inline";
    case kInlineInsideDocumentWrite:
      return "inline inside document write";
    case kInlineInsideGeneratedElement:
      return "inline inside generated element";
    case kInternal:
      return "internal";
    case kJavascriptUrl:
      return "javascript url";
    case kEvalForScheduledAction:
      return "eval for scheduled action";
    case kInspector:
      return "inspector";
    case kScriptTypeUnknown:
    default:
      return "unknown";
  }
}

string RequestStatusToString(const RequestStatus status) noexcept {
  switch (status) {
    case kRequestStatusStart:
      return "started";
    case kRequestStatusComplete:
      return "complete";
    case kRequestStatusError:
      return "error";
    case kRequestStatusBlocked:
      return "blocked";
  }
}

string StorageLocationToString(const StorageLocation location) noexcept {
  switch (location) {
    case kStorageLocationCookie:
      return "cookie";
    case kStorageLocationLocalStorage:
      return "localStorage";
    case kStorageLocationSessionStorage:
      return "sessionStorage";
  }
}

namespace {
  const map<JSBuiltIn, string> js_built_in_enum_to_str_map = {
    {kJSBuiltInDateNow, "Date.now"},
    {kJSBuiltInDateConstructor, "new Date()"},
    {kJSBuiltInDateParse, "Date.parse"},
    {kJSBuiltInDateUTC, "Date.UTC"},
    {kJSBuiltInDatePrototypeSetDate, "Date.prototype.setDate"},
    {kJSBuiltInDatePrototypeSetFullYear, "Date.prototype.setFullYear"},
    {kJSBuiltInDatePrototypeSetHours, "Date.prototype.setHours"},
    {kJSBuiltInDatePrototypeSetMilliseconds, "Date.prototype.setMilliseconds"},
    {kJSBuiltInDatePrototypeSetMinutes, "Date.prototype.setMinutes"},
    {kJSBuiltInDatePrototypeSetMonth, "Date.prototype.setMonth"},
    {kJSBuiltInDatePrototypeSetSeconds, "Date.prototype.setSeconds"},
    {kJSBuiltInDatePrototypeSetTime, "Date.prototype.setTime"},
    {kJSBuiltInDatePrototypeSetUTCDate, "Date.prototype.setUTCDate"},
    {kJSBuiltInDatePrototypeSetUTCFullYear, "Date.prototype.setUTCFullYear"},
    {kJSBuiltInDatePrototypeSetUTCHours, "Date.prototype.setUTCHours"},
    {kJSBuiltInDatePrototypeSetUTCMilliseconds, "Date.prototype.setUTCMilliseconds"},
    {kJSBuiltInDatePrototypeSetUTCMinutes, "Date.prototype.setUTCMinutes"},
    {kJSBuiltInDatePrototypeSetUTCMonth, "Date.prototype.setUTCMonth"},
    {kJSBuiltInDatePrototypeSetUTCSeconds, "Date.prototype.setUTCSeconds"},
    {kJSBuiltInDatePrototypeToDateString, "Date.prototype.toDateString"},
    {kJSBuiltInDatePrototypeToISOString, "Date.prototype.toISOString"},
    {kJSBuiltInDatePrototypeToString, "Date.prototype.toString"},
    {kJSBuiltInDatePrototypeToTimeString, "Date.prototype.toTimeString"},
    {kJSBuiltInDatePrototypeToLocaleDateString, "Date.prototype.toLocaleDateString"},
    {kJSBuiltInDatePrototypeToLocalString, "Date.prototype.toLocaleString"},
    {kJSBuiltInDatePrototypeToLocalTimeString, "Date.prototype.toLocaleTimeString"},
    {kJSBuiltInDatePrototypeToUTCString, "Date.prototype.toUTCString"},
    {kJSBuiltInDatePrototypeGetYear, "Date.prototype.getYear"},
    {kJSBuiltInDatePrototypeSetYear, "Date.prototype.setYear"},
    {kJSBuiltInDatePrototypeToJSON, "Date.prototype.toJSON"},
    {kJSBuiltInJSONParse, "JSON.parse"},
    {kJSBuiltInJSONStringify, "JSON.stringify"},
  };

  const map<string, JSBuiltIn> js_built_in_str_to_enum_map = {
    {"Date.now", kJSBuiltInDateNow},
    {"new Date()", kJSBuiltInDateConstructor},
    {"Date.parse", kJSBuiltInDateParse},
    {"Date.UTC", kJSBuiltInDateUTC},
    {"Date.prototype.setDate", kJSBuiltInDatePrototypeSetDate},
    {"Date.prototype.setFullYear", kJSBuiltInDatePrototypeSetFullYear},
    {"Date.prototype.setHours", kJSBuiltInDatePrototypeSetHours},
    {"Date.prototype.setMilliseconds", kJSBuiltInDatePrototypeSetMilliseconds},
    {"Date.prototype.setMinutes", kJSBuiltInDatePrototypeSetMinutes},
    {"Date.prototype.setMonth", kJSBuiltInDatePrototypeSetMonth},
    {"Date.prototype.setSeconds", kJSBuiltInDatePrototypeSetSeconds},
    {"Date.prototype.setTime", kJSBuiltInDatePrototypeSetTime},
    {"Date.prototype.setUTCDate", kJSBuiltInDatePrototypeSetUTCDate},
    {"Date.prototype.setUTCFullYear", kJSBuiltInDatePrototypeSetUTCFullYear},
    {"Date.prototype.setUTCHours", kJSBuiltInDatePrototypeSetUTCHours},
    {"Date.prototype.setUTCMilliseconds", kJSBuiltInDatePrototypeSetUTCMilliseconds},
    {"Date.prototype.setUTCMinutes", kJSBuiltInDatePrototypeSetUTCMinutes},
    {"Date.prototype.setUTCMonth", kJSBuiltInDatePrototypeSetUTCMonth},
    {"Date.prototype.setUTCSeconds", kJSBuiltInDatePrototypeSetUTCSeconds},
    {"Date.prototype.toDateString", kJSBuiltInDatePrototypeToDateString},
    {"Date.prototype.toISOString", kJSBuiltInDatePrototypeToISOString},
    {"Date.prototype.toString", kJSBuiltInDatePrototypeToString},
    {"Date.prototype.toTimeString", kJSBuiltInDatePrototypeToTimeString},
    {"Date.prototype.toLocaleDateString", kJSBuiltInDatePrototypeToLocaleDateString},
    {"Date.prototype.toLocaleString", kJSBuiltInDatePrototypeToLocalString},
    {"Date.prototype.toLocaleTimeString", kJSBuiltInDatePrototypeToLocalTimeString},
    {"Date.prototype.toUTCString", kJSBuiltInDatePrototypeToUTCString},
    {"Date.prototype.getYear", kJSBuiltInDatePrototypeGetYear},
    {"Date.prototype.setYear", kJSBuiltInDatePrototypeSetYear},
    {"Date.prototype.toJSON", kJSBuiltInDatePrototypeToJSON},
    {"JSON.parse", kJSBuiltInJSONParse},
    {"JSON.stringify", kJSBuiltInJSONStringify},
  };
}

JSBuiltIn JSBuiltInFromString(const string& built_in_name) noexcept {
  LOG_ASSERT(js_built_in_str_to_enum_map.count(built_in_name) != 0);
  return js_built_in_str_to_enum_map.at(built_in_name);
}

const string& JSBuiltInToSting(const JSBuiltIn built_in) noexcept {
  LOG_ASSERT(js_built_in_enum_to_str_map.count(built_in) != 0);
  return js_built_in_enum_to_str_map.at(built_in);
}

namespace {
  const map<WebAPI, string> web_api_enum_to_str_map = {
    {kWebAPIDocumentReferrer, "Document.referrer"},
    {kWebAPILocationAncestorOrigins, "Location.ancestorOrigins"},
    {kWebAPILocationAssign, "Location.assign"},
    {kWebAPILocationHash, "Location.hash"},
    {kWebAPILocationHost, "Location.host"},
    {kWebAPILocationHostname, "Location.hostname"},
    {kWebAPILocationHref, "Location.href"},
    {kWebAPILocationOrigin, "Location.origin"},
    {kWebAPILocationPathname, "Location.pathname"},
    {kWebAPILocationPort, "Location.port"},
    {kWebAPILocationProtocol, "Location.protocol"},
    {kWebAPILocationReload, "Location.reload"},
    {kWebAPILocationReplace, "Location.replace"},
    {kWebAPILocationSearch, "Location.search"},
  };

  const map<string, WebAPI> web_api_str_to_enum_map = {
    {"Document.referrer", kWebAPIDocumentReferrer},
    {"Location.ancestorOrigins", kWebAPILocationAncestorOrigins},
    {"Location.assign", kWebAPILocationAssign},
    {"Location.hash", kWebAPILocationHash},
    {"Location.host", kWebAPILocationHost},
    {"Location.hostname", kWebAPILocationHostname},
    {"Location.href", kWebAPILocationHref},
    {"Location.origin", kWebAPILocationOrigin},
    {"Location.pathname", kWebAPILocationPathname},
    {"Location.port", kWebAPILocationPort},
    {"Location.protocol", kWebAPILocationProtocol},
    {"Location.reload", kWebAPILocationReload},
    {"Location.replace", kWebAPILocationReplace},
    {"Location.search", kWebAPILocationSearch},
  };
}

WebAPI WebAPIFromString(const string& web_api_name) noexcept {
  LOG_ASSERT(web_api_str_to_enum_map.count(web_api_name) != 0);
  return web_api_str_to_enum_map.at(web_api_name);
}

const string& WebAPIToString(const WebAPI web_api) noexcept {
  LOG_ASSERT(web_api_enum_to_str_map.count(web_api) != 0);
  return web_api_enum_to_str_map.at(web_api);
}

FingerprintingRule::FingerprintingRule(const std::string& primary_pattern,
                                       const std::string& secondary_pattern,
                                       const std::string& source,
                                       const bool incognito) :
  primary_pattern(primary_pattern),
  secondary_pattern(secondary_pattern),
  source(source),
  incognito(incognito) {}

bool FingerprintingRule::operator==(const FingerprintingRule& other) const {
  return primary_pattern == other.primary_pattern
    && secondary_pattern == other.secondary_pattern
    && source == other.source
    && incognito == other.incognito;
}

bool FingerprintingRule::operator<(const FingerprintingRule& other) const {
  return primary_pattern < other.primary_pattern
    && secondary_pattern < other.secondary_pattern
    && source < other.source
    && incognito < other.incognito;
}

bool FingerprintingRule::operator>(const FingerprintingRule& other) const {
  return primary_pattern > other.primary_pattern
    && secondary_pattern > other.secondary_pattern
    && source > other.source
    && incognito > other.incognito;
}

std::string FingerprintingRule::ToString() const {
  return "primary pattern: " + primary_pattern
    + ", secondary pattern: " + secondary_pattern
    + ", source: " + source
    + ", incognito: " + std::to_string(incognito);
}

}  // namespace brave_page_graph
