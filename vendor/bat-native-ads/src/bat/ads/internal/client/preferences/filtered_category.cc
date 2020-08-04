/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/client/preferences/filtered_category.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

FilteredCategory::FilteredCategory() = default;

FilteredCategory::FilteredCategory(
    const FilteredCategory& category) = default;

FilteredCategory::~FilteredCategory() = default;

std::string FilteredCategory::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result FilteredCategory::FromJson(
    const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return FAILED;
  }

  if (document.HasMember("name")) {
    name = document["name"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(
    JsonWriter* writer,
    const FilteredCategory& category) {
  writer->StartObject();

  writer->String("name");
  writer->String(category.name.c_str());

  writer->EndObject();
}

}  // namespace ads
