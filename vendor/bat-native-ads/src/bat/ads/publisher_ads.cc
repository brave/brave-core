/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/publisher_ads.h"
#include "bat/ads/internal/json_helper.h"
#include "base/logging.h"

namespace ads {

PublisherAds::PublisherAds() = default;

PublisherAds::PublisherAds(
    const PublisherAds& info) = default;

PublisherAds::~PublisherAds() = default;

const std::string PublisherAds::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result PublisherAds::FromJson(
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

  if (document.HasMember("publisher_ads")) {
    for (const auto& entry : document["publisher_ads"].GetArray()) {
      PublisherAdInfo ad;
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      if (entry.Accept(writer) && ad.FromJson(buffer.GetString()) == SUCCESS) {
        entries.push_back(ad);
      }
    }
  }

  return SUCCESS;
}

void SaveToJson(
    JsonWriter* writer,
    const PublisherAds& ads) {
  writer->StartObject();

  writer->String("publisher_ads");
  writer->StartArray();
  for (const auto& entry : ads.entries) {
    SaveToJson(writer, entry);
  }
  writer->EndArray();

  writer->EndObject();
}

}  // namespace ads
