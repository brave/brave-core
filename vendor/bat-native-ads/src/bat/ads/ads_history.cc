/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads_history.h"

#include "bat/ads/ad_history_detail.h"
#include "bat/ads/internal/json_helper.h"

#include "base/logging.h"

namespace ads {

AdsHistory::AdsHistory() :
    details() {}

AdsHistory::AdsHistory(const AdsHistory& history) :
    details(history.details) {}

AdsHistory::~AdsHistory() = default;

const std::string AdsHistory::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result AdsHistory::FromJson(
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

  if (document.HasMember("details")) {
    for (const auto& detail : document["details"].GetArray()) {
      AdHistoryDetail ad_history_detail;
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      if (detail.Accept(writer) &&
          ad_history_detail.FromJson(buffer.GetString()) == SUCCESS) {
        details.push_back(ad_history_detail);
      }
    }
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const AdsHistory& history) {
  writer->StartObject();

  writer->String("details");
  writer->StartArray();
  for (const auto& ad_history_detail : history.details) {
    SaveToJson(writer, ad_history_detail);
  }
  writer->EndArray();

  writer->EndObject();
}

}  // namespace ads
