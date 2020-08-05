/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_JSON_HELPER_H_
#define BAT_ADS_INTERNAL_JSON_HELPER_H_

#ifdef _MSC_VER
// Resolve Windows build issue due to Windows globally defining GetObject which
// causes RapidJson to fail
#undef GetObject
#endif

#include <string>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/schema.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/result.h"

namespace ads {

struct AdContent;
struct AdHistory;
struct AdNotificationInfo;
struct AdPreferences;
struct AdsHistory;
struct CategoryContent;
struct ClientState;
struct PurchaseIntentSignalHistory;

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

void SaveToJson(
    JsonWriter* writer,
    const AdContent& content);
void SaveToJson(
    JsonWriter* writer,
    const AdHistory& history);
void SaveToJson(
    JsonWriter* writer,
    const AdNotificationInfo& info);
void SaveToJson(
    JsonWriter* writer,
    const AdPreferences& prefs);
void SaveToJson(
    JsonWriter* writer,
    const AdsHistory& history);
void SaveToJson(
    JsonWriter* writer,
    const CategoryContent& content);
void SaveToJson(
    JsonWriter* writer,
    const ClientState& state);
void SaveToJson(
    JsonWriter* writer,
    const PurchaseIntentSignalHistory& info);

template <typename T>
void SaveToJson(
    const T& t,
    std::string* json) {
  DCHECK(json);

  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  SaveToJson(&writer, t);
  *json = buffer.GetString();
}

template <typename T>
Result LoadFromJson(
    T* t,
    const std::string& json) {
  DCHECK(t);
  return t->FromJson(json);
}

template <typename T>
Result LoadFromJson(
    T* t,
    const std::string& json,
    const std::string& json_schema) {
  DCHECK(t);
  return t->FromJson(json, json_schema);
}

}  // namespace ads

namespace helper {

class JSON {
 public:
  static ads::Result Validate(
      rapidjson::Document* document,
      const std::string& json_schema);

  static std::string GetLastError(rapidjson::Document* document);
};

}  // namespace helper

#endif  // BAT_ADS_INTERNAL_JSON_HELPER_H_
