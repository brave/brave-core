/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client/preferences/filtered_advertiser_info.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

FilteredAdvertiserInfo::FilteredAdvertiserInfo() = default;

FilteredAdvertiserInfo::FilteredAdvertiserInfo(
    const FilteredAdvertiserInfo& info) = default;

FilteredAdvertiserInfo::~FilteredAdvertiserInfo() = default;

std::string FilteredAdvertiserInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool FilteredAdvertiserInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("id")) {
    id = document["id"].GetString();
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const FilteredAdvertiserInfo& info) {
  writer->StartObject();

  writer->String("id");
  writer->String(info.id.c_str());

  writer->EndObject();
}

}  // namespace ads
