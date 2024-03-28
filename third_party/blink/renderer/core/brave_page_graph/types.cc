/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"

#include <map>
#include <string>

#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"

namespace brave_page_graph {

FingerprintingRule::FingerprintingRule(const std::string& primary_pattern,
                                       const std::string& secondary_pattern,
                                       const std::string& source,
                                       const bool incognito)
    : primary_pattern(primary_pattern),
      secondary_pattern(secondary_pattern),
      source(source),
      incognito(incognito) {}

bool FingerprintingRule::operator==(const FingerprintingRule& other) const {
  return primary_pattern == other.primary_pattern &&
         secondary_pattern == other.secondary_pattern &&
         source == other.source && incognito == other.incognito;
}

bool FingerprintingRule::operator<(const FingerprintingRule& other) const {
  return primary_pattern < other.primary_pattern &&
         secondary_pattern < other.secondary_pattern && source < other.source &&
         incognito < other.incognito;
}

bool FingerprintingRule::operator>(const FingerprintingRule& other) const {
  return primary_pattern > other.primary_pattern &&
         secondary_pattern > other.secondary_pattern && source > other.source &&
         incognito > other.incognito;
}

std::string FingerprintingRule::ToString() const {
  return "primary pattern: " + primary_pattern +
         ", secondary pattern: " + secondary_pattern + ", source: " + source +
         ", incognito: " + base::NumberToString(incognito);
}

bool ScriptSource::operator==(const ScriptSource& rhs) const {
  auto tie = [](const ScriptSource& v) {
    return std::tie(v.dom_node_id, v.parent_script_id, v.url, v.function_name,
                    v.location_type, v.is_module, v.is_eval);
  };
  return tie(*this) == tie(rhs);
}

bool ScriptData::operator==(const ScriptData& rhs) const {
  auto tie = [](const ScriptData& v) { return std::tie(v.code, v.source); };
  return tie(*this) == tie(rhs);
}

const char* BindingTypeToString(blink::PageGraphBindingType binding_type) {
  switch (binding_type) {
    case blink::PageGraphBindingType::kAttribute:
      return "attribute";
    case blink::PageGraphBindingType::kConstant:
      return "constant";
    case blink::PageGraphBindingType::kConstructor:
      return "constructor";
    case blink::PageGraphBindingType::kMethod:
      return "method";
  }
}

const char* BindingEventToString(blink::PageGraphBindingEvent binding_event) {
  switch (binding_event) {
    case blink::PageGraphBindingEvent::kAttributeGet:
      return "attribute get";
    case blink::PageGraphBindingEvent::kAttributeSet:
      return "attribute set";
    case blink::PageGraphBindingEvent::kConstantGet:
      return "constant get";
    case blink::PageGraphBindingEvent::kConstructorCall:
      return "constructor call";
    case blink::PageGraphBindingEvent::kMethodCall:
      return "method call";
  }
}

std::string GraphMLAttrTypeToString(const GraphMLAttrType type) {
  switch (type) {
    case kGraphMLAttrTypeString:
      return "string";
    case kGraphMLAttrTypeBoolean:
      return "boolean";
    case kGraphMLAttrTypeInt:
      return "int";
    case kGraphMLAttrTypeFloat:
      return "float";
    case kGraphMLAttrTypeDouble:
      return "double";
    case kGraphMLAttrTypeUnknown:
    default:
      return "unknown";
  }
}

std::string GraphMLForTypeToString(const GraphMLAttrForType type) {
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

std::string RequestStatusToString(const RequestStatus status) {
  switch (status) {
    case kRequestStatusStart:
      return "started";
    case kRequestStatusComplete:
      return "complete";
    case kRequestStatusRedirect:
      return "redirect";
    case kRequestStatusError:
      return "error";
    case kRequestStatusBlocked:
      return "blocked";
  }
}

std::string StorageLocationToString(const StorageLocation location) {
  switch (location) {
    case StorageLocation::kCookie:
      return "cookie";
    case StorageLocation::kLocalStorage:
      return "localStorage";
    case StorageLocation::kSessionStorage:
      return "sessionStorage";
  }
}

}  // namespace brave_page_graph
