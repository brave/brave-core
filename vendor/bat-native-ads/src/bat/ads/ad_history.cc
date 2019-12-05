/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_history.h"

#include "bat/ads/ad_content.h"
#include "bat/ads/category_content.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/time.h"

#include "base/logging.h"

namespace ads {

AdHistory::AdHistory()
    : timestamp_in_seconds(0) {}

AdHistory::AdHistory(const AdHistory& properties)
    : timestamp_in_seconds(properties.timestamp_in_seconds),
      uuid(properties.uuid),
      ad_content(properties.ad_content),
      category_content(properties.category_content) {}

AdHistory::~AdHistory() = default;

const std::string AdHistory::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result AdHistory::FromJson(
    const std::string& json,
    std::string* error_description) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    if (error_description != nullptr) {
      *error_description = helper::JSON::GetLastError(&document);
    }

    return FAILED;
  }

  if (document.HasMember("timestamp_in_seconds")) {
    auto migrated_timestamp_in_seconds = Time::MigrateTimestampToDoubleT(
        document["timestamp_in_seconds"].GetUint64());
    timestamp_in_seconds = migrated_timestamp_in_seconds;
  }

  if (document.HasMember("uuid")) {
    uuid = document["uuid"].GetString();
  }

  if (document.HasMember("ad_content")) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    const auto& value = document["ad_content"];
    if (!value.Accept(writer) ||
        ad_content.FromJson(buffer.GetString()) != SUCCESS) {
      return FAILED;
    }
  }

  if (document.HasMember("category_content")) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    const auto& value = document["category_content"];
    if (!value.Accept(writer) ||
        category_content.FromJson(buffer.GetString()) != SUCCESS) {
      return FAILED;
    }
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const AdHistory& history) {
  writer->StartObject();

  writer->String("timestamp_in_seconds");
  writer->Uint64(history.timestamp_in_seconds);

  writer->String("uuid");
  writer->String(history.uuid.c_str());

  writer->String("ad_content");
  SaveToJson(writer, history.ad_content);

  writer->String("category_content");
  SaveToJson(writer, history.category_content);

  writer->EndObject();
}

}  // namespace ads
