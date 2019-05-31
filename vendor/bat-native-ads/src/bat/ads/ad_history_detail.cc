/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_history_detail.h"

#include "bat/ads/ad_content.h"
#include "bat/ads/category_content.h"

#include "bat/ads/internal/json_helper.h"

#include "base/logging.h"

namespace ads {

AdHistoryDetail::AdHistoryDetail() :
    timestamp_in_seconds(0),
    uuid(""),
    ad_content(),
    category_content() {}

AdHistoryDetail::AdHistoryDetail(const AdHistoryDetail& detail) :
    timestamp_in_seconds(detail.timestamp_in_seconds),
    uuid(detail.uuid),
    ad_content(detail.ad_content),
    category_content(detail.category_content) {}

AdHistoryDetail::~AdHistoryDetail() = default;

const std::string AdHistoryDetail::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result AdHistoryDetail::FromJson(
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
    timestamp_in_seconds = document["timestamp_in_seconds"].GetUint64();
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

void SaveToJson(JsonWriter* writer, const AdHistoryDetail& detail) {
  writer->StartObject();

  writer->String("timestamp_in_seconds");
  writer->Uint64(detail.timestamp_in_seconds);

  writer->String("uuid");
  writer->String(detail.uuid.c_str());

  writer->String("ad_content");
  SaveToJson(writer, detail.ad_content);

  writer->String("category_content");
  SaveToJson(writer, detail.category_content);

  writer->EndObject();
}

}  // namespace ads
