/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_JSON_HELPER_H_
#define BAT_ADS_INTERNAL_JSON_HELPER_H_

#include <string>

#include "bat/ads/result.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/schema.h"

namespace ads {

struct AdContent;
struct AdHistory;
struct CreativeAdNotificationInfo;
struct AdConversionTrackingInfo;
struct CreativePublisherAdInfo;
struct AdPreferences;
struct AdsHistory;
struct PublisherAds;
struct PublisherAdInfo;
struct BundleState;
struct CategoryContent;
struct ClientInfo;
struct ClientState;
struct IssuersInfo;
struct AdNotificationInfo;

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

void SaveToJson(JsonWriter* writer, const AdContent& content);
void SaveToJson(JsonWriter* writer, const AdHistory& history);
void SaveToJson(JsonWriter* writer, const CreativeAdNotificationInfo& info);
void SaveToJson(JsonWriter* writer, const AdConversionTrackingInfo& info);
void SaveToJson(JsonWriter* writer, const CreativePublisherAdInfo& info);
void SaveToJson(JsonWriter* writer, const AdPreferences& prefs);
void SaveToJson(JsonWriter* writer, const AdsHistory& history);
void SaveToJson(JsonWriter* writer, const PublisherAds& ads);
void SaveToJson(JsonWriter* writer, const PublisherAdInfo& ads);
void SaveToJson(JsonWriter* writer, const BundleState& state);
void SaveToJson(JsonWriter* writer, const CategoryContent& content);
void SaveToJson(JsonWriter* writer, const ClientInfo& info);
void SaveToJson(JsonWriter* writer, const ClientState& state);
void SaveToJson(JsonWriter* writer, const IssuersInfo& info);
void SaveToJson(JsonWriter* writer, const AdNotificationInfo& info);

template <typename T>
void SaveToJson(const T& t, std::string* json) {
  if (!json) {
    return;
  }

  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  SaveToJson(&writer, t);
  *json = buffer.GetString();
}

template <typename T>
Result LoadFromJson(
    T* t,
    const std::string& json,
    std::string* error_description) {
  return t->FromJson(json, error_description);
}

template <typename T>
Result LoadFromJson(
    T* t,
    const std::string& json,
    const std::string& json_schema,
    std::string* error_description) {
  return t->FromJson(json, json_schema, error_description);
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
