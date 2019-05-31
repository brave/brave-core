/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/category_content.h"

#include "bat/ads/internal/json_helper.h"

#include "base/logging.h"

namespace ads {

CategoryContent::CategoryContent() :
    category(""),
    opt_action(0) {}

CategoryContent::CategoryContent(const CategoryContent& content) :
    category(content.category),
    opt_action(content.opt_action) {}

CategoryContent::~CategoryContent() = default;

const std::string CategoryContent::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result CategoryContent::FromJson(
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

  if (document.HasMember("category")) {
    category = document["category"].GetString();
  }

  if (document.HasMember("opt_action")) {
    opt_action = document["opt_action"].GetInt();
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const CategoryContent& content) {
  writer->StartObject();

  writer->String("category");
  writer->String(content.category.c_str());

  writer->String("opt_action");
  writer->Int(content.opt_action);

  writer->EndObject();
}

}  // namespace ads
