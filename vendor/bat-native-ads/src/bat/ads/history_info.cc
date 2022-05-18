/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/history_info.h"

#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/json_helper.h"

namespace ads {

HistoryInfo::HistoryInfo() = default;

HistoryInfo::HistoryInfo(const HistoryInfo& info) = default;

HistoryInfo::~HistoryInfo() = default;

std::string HistoryInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool HistoryInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("history")) {
    for (const auto& item : document["history"].GetArray()) {
      HistoryItemInfo history_item;
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      if (item.Accept(writer) && history_item.FromJson(buffer.GetString())) {
        items.push_back(history_item);
      }
    }
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const HistoryInfo& info) {
  writer->StartObject();

  writer->String("history");
  writer->StartArray();
  for (const auto& item : info.items) {
    SaveToJson(writer, item);
  }
  writer->EndArray();

  writer->EndObject();
}

}  // namespace ads
