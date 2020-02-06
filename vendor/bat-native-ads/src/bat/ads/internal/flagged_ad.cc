/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flagged_ad.h"

#include "bat/ads/internal/json_helper.h"

namespace ads {

FlaggedAd::FlaggedAd() = default;

FlaggedAd::FlaggedAd(
    const FlaggedAd& ad) = default;

FlaggedAd::~FlaggedAd() = default;

const std::string FlaggedAd::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result FlaggedAd::FromJson(
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

  if (document.HasMember("uuid")) {
    uuid = document["uuid"].GetString();
  }

  if (document.HasMember("creative_set_id")) {
    creative_set_id = document["creative_set_id"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const FlaggedAd& ad) {
  writer->StartObject();

  writer->String("uuid");
  writer->String(ad.uuid.c_str());

  writer->String("creative_set_id");
  writer->String(ad.creative_set_id.c_str());

  writer->EndObject();
}

}  // namespace ads
