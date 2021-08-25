/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/category_content_info.h"

#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

CategoryContentInfo::CategoryContentInfo() = default;

CategoryContentInfo::CategoryContentInfo(const CategoryContentInfo& info) =
    default;

CategoryContentInfo::~CategoryContentInfo() = default;

bool CategoryContentInfo::operator==(const CategoryContentInfo& rhs) const {
  return category == rhs.category && opt_action == rhs.opt_action;
}

bool CategoryContentInfo::operator!=(const CategoryContentInfo& rhs) const {
  return !(*this == rhs);
}

std::string CategoryContentInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool CategoryContentInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("category")) {
    category = document["category"].GetString();
  }

  if (document.HasMember("opt_action")) {
    opt_action = static_cast<OptAction>(document["opt_action"].GetInt());
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const CategoryContentInfo& content) {
  writer->StartObject();

  writer->String("category");
  writer->String(content.category.c_str());

  writer->String("opt_action");
  writer->Int(static_cast<int>(content.opt_action));

  writer->EndObject();
}

}  // namespace ads
