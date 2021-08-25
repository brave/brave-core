/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client/preferences/filtered_category_info.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

FilteredCategoryInfo::FilteredCategoryInfo() = default;

FilteredCategoryInfo::FilteredCategoryInfo(
    const FilteredCategoryInfo& category) = default;

FilteredCategoryInfo::~FilteredCategoryInfo() = default;

std::string FilteredCategoryInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool FilteredCategoryInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("name")) {
    name = document["name"].GetString();
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const FilteredCategoryInfo& category) {
  writer->StartObject();

  writer->String("name");
  writer->String(category.name.c_str());

  writer->EndObject();
}

}  // namespace ads
