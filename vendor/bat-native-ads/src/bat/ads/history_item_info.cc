/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/history_item_info.h"

#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/number_util.h"
#include "bat/ads/internal/deprecated/json_helper.h"

namespace ads {

HistoryItemInfo::HistoryItemInfo() = default;

HistoryItemInfo::HistoryItemInfo(const HistoryItemInfo& info) = default;

HistoryItemInfo::~HistoryItemInfo() = default;

bool HistoryItemInfo::operator==(const HistoryItemInfo& rhs) const {
  return DoubleEquals(time.ToDoubleT(), rhs.time.ToDoubleT()) &&
         ad_content == rhs.ad_content &&
         category_content == rhs.category_content;
}

bool HistoryItemInfo::operator!=(const HistoryItemInfo& rhs) const {
  return !(*this == rhs);
}

std::string HistoryItemInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool HistoryItemInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("timestamp_in_seconds")) {
    time =
        base::Time::FromDoubleT(document["timestamp_in_seconds"].GetDouble());
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

void SaveToJson(JsonWriter* writer, const HistoryItemInfo& info) {
  writer->StartObject();

  writer->String("timestamp_in_seconds");
  writer->Double(info.time.ToDoubleT());

  writer->String("ad_content");
  SaveToJson(writer, info.ad_content);

  writer->String("category_content");
  SaveToJson(writer, info.category_content);

  writer->EndObject();
}

}  // namespace ads
