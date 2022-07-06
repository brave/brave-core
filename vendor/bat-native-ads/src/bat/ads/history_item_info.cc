/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/history_item_info.h"

#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/numbers/number_util.h"
#include "bat/ads/internal/deprecated/json/json_helper.h"

namespace ads {

HistoryItemInfo::HistoryItemInfo() = default;

HistoryItemInfo::HistoryItemInfo(const HistoryItemInfo& info) = default;

HistoryItemInfo& HistoryItemInfo::operator=(const HistoryItemInfo& info) =
    default;

HistoryItemInfo::~HistoryItemInfo() = default;

bool HistoryItemInfo::operator==(const HistoryItemInfo& rhs) const {
  return DoubleEquals(created_at.ToDoubleT(), rhs.created_at.ToDoubleT()) &&
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
    if (document["timestamp_in_seconds"].IsNumber()) {
      // Migrate legacy timestamp
      created_at =
          base::Time::FromDoubleT(document["timestamp_in_seconds"].GetDouble());
    } else {
      double timestamp = 0.0;
      if (base::StringToDouble(document["timestamp_in_seconds"].GetString(),
                               &timestamp)) {
        created_at = base::Time::FromDoubleT(timestamp);
      }
    }
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
  const std::string created_at =
      base::NumberToString(info.created_at.ToDoubleT());
  writer->String(created_at.c_str());

  writer->String("ad_content");
  SaveToJson(writer, info.ad_content);

  writer->String("category_content");
  SaveToJson(writer, info.category_content);

  writer->EndObject();
}

}  // namespace ads
