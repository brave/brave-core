/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client/preferences/saved_ad_info.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

SavedAdInfo::SavedAdInfo() = default;

SavedAdInfo::SavedAdInfo(const SavedAdInfo& ad) = default;

SavedAdInfo::~SavedAdInfo() = default;

std::string SavedAdInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool SavedAdInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("uuid")) {
    creative_instance_id = document["uuid"].GetString();
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const SavedAdInfo& info) {
  writer->StartObject();

  writer->String("uuid");
  writer->String(info.creative_instance_id.c_str());

  writer->EndObject();
}

}  // namespace ads
