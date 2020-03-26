/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_info.h"
#include "bat/ads/internal/json_helper.h"

namespace ads {

AdInfo::AdInfo() = default;

AdInfo::AdInfo(
    const AdInfo& info) = default;

AdInfo::~AdInfo() = default;

std::string AdInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result AdInfo::FromJson(
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
    creative_instance_id = document["uuid"].GetString();
  }

  if (document.HasMember("creative_set_id")) {
    creative_set_id = document["creative_set_id"].GetString();
  }

  if (document.HasMember("category")) {
    category = document["category"].GetString();
  }

  if (document.HasMember("url")) {
    target_url = document["url"].GetString();
  }

  if (document.HasMember("geo_target")) {
    geo_target = document["geo_target"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(
    JsonWriter* writer,
    const AdInfo& info) {
  writer->StartObject();

  writer->String("uuid");
  writer->String(info.creative_instance_id.c_str());

  writer->String("creative_set_id");
  writer->String(info.creative_set_id.c_str());

  writer->String("category");
  writer->String(info.category.c_str());

  writer->String("url");
  writer->String(info.target_url.c_str());

  writer->String("geo_target");
  writer->String(info.geo_target.c_str());

  writer->EndObject();
}

}  // namespace ads
