/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/filtered_category.h"

#include "bat/ads/internal/json_helper.h"

namespace ads {

FilteredCategory::FilteredCategory() = default;

FilteredCategory::FilteredCategory(
    const FilteredCategory& category) = default;

FilteredCategory::~FilteredCategory() = default;

const std::string FilteredCategory::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result FilteredCategory::FromJson(
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

  if (document.HasMember("name")) {
    name = document["name"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const FilteredCategory& category) {
  writer->StartObject();

  writer->String("name");
  writer->String(category.name.c_str());

  writer->EndObject();
}

}  // namespace ads
