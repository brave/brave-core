/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_JSON_HELPER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_JSON_HELPER_H_

#ifdef _MSC_VER
// Resolve Windows build issue due to Windows globally defining GetObject which
// causes RapidJson to fail
#undef GetObject
#endif

#include <string>

#include "base/check.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/schema.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace ads {

struct AdContentInfo;
struct AdHistoryInfo;
struct AdNotificationInfo;
struct AdPreferencesInfo;
struct AdsHistoryInfo;
struct CategoryContentInfo;
struct ClientInfo;
struct NewTabPageAdInfo;

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

void SaveToJson(JsonWriter* writer, const AdContentInfo& info);
void SaveToJson(JsonWriter* writer, const AdHistoryInfo& info);
void SaveToJson(JsonWriter* writer, const AdNotificationInfo& info);
void SaveToJson(JsonWriter* writer, const AdPreferencesInfo& info);
void SaveToJson(JsonWriter* writer, const AdsHistoryInfo& info);
void SaveToJson(JsonWriter* writer, const CategoryContentInfo& info);
void SaveToJson(JsonWriter* writer, const ClientInfo& state);
void SaveToJson(JsonWriter* writer, const NewTabPageAdInfo& info);

template <typename T>
void SaveToJson(const T& t, std::string* json) {
  DCHECK(json);

  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  SaveToJson(&writer, t);
  *json = buffer.GetString();
}

template <typename T>
bool LoadFromJson(T* t, const std::string& json) {
  DCHECK(t);
  return t->FromJson(json);
}

template <typename T>
bool LoadFromJson(T* t,
                  const std::string& json,
                  const std::string& json_schema) {
  DCHECK(t);
  return t->FromJson(json, json_schema);
}

}  // namespace ads

namespace helper {

class JSON {
 public:
  static bool Validate(rapidjson::Document* document,
                       const std::string& json_schema);

  static std::string GetLastError(rapidjson::Document* document);
};

}  // namespace helper

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_JSON_HELPER_H_
