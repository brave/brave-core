/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_history_info.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/number_util.h"

namespace ads {

AdHistoryInfo::AdHistoryInfo() = default;

AdHistoryInfo::AdHistoryInfo(const AdHistoryInfo& info) = default;

AdHistoryInfo::~AdHistoryInfo() = default;

bool AdHistoryInfo::operator==(const AdHistoryInfo& rhs) const {
  return DoubleEquals(timestamp, rhs.timestamp) &&
         ad_content == rhs.ad_content &&
         category_content == rhs.category_content;
}

bool AdHistoryInfo::operator!=(const AdHistoryInfo& rhs) const {
  return !(*this == rhs);
}

std::string AdHistoryInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool AdHistoryInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("timestamp_in_seconds")) {
    timestamp = document["timestamp_in_seconds"].GetDouble();
  }

  if (document.HasMember("ad_content")) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    const auto& value = document["ad_content"];
    if (!value.Accept(writer) || !ad_content.FromJson(buffer.GetString())) {
      return false;
    }
  }

  if (document.HasMember("category_content")) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    const auto& value = document["category_content"];
    if (!value.Accept(writer) ||
        !category_content.FromJson(buffer.GetString())) {
      return false;
    }
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const AdHistoryInfo& ad_history) {
  writer->StartObject();

  writer->String("timestamp_in_seconds");
  writer->Double(ad_history.timestamp);

  writer->String("ad_content");
  SaveToJson(writer, ad_history.ad_content);

  writer->String("category_content");
  SaveToJson(writer, ad_history.category_content);

  writer->EndObject();
}

}  // namespace ads
