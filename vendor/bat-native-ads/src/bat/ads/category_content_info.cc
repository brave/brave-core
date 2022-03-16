/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/category_content_info.h"

#include "base/values.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

CategoryContentInfo::CategoryContentInfo() = default;

CategoryContentInfo::CategoryContentInfo(const CategoryContentInfo& info) =
    default;

CategoryContentInfo::~CategoryContentInfo() = default;

bool CategoryContentInfo::operator==(const CategoryContentInfo& rhs) const {
  return category == rhs.category && opt_action_type == rhs.opt_action_type;
}

bool CategoryContentInfo::operator!=(const CategoryContentInfo& rhs) const {
  return !(*this == rhs);
}

base::DictionaryValue CategoryContentInfo::ToValue() const {
  base::DictionaryValue dictionary;

  dictionary.SetStringKey("category", category);
  dictionary.SetIntKey("optAction", static_cast<int>(opt_action_type));

  return dictionary;
}

bool CategoryContentInfo::FromValue(const base::Value& value) {
  const base::DictionaryValue* dictionary = nullptr;
  if (!(&value)->GetAsDictionary(&dictionary)) {
    return false;
  }

  const std::string* category_value = dictionary->FindStringKey("category");
  if (category_value) {
    category = *category_value;
  }

  const absl::optional<int> opt_action_type_optional =
      dictionary->FindIntKey("optAction");
  if (opt_action_type_optional) {
    opt_action_type = static_cast<CategoryContentOptActionType>(
        opt_action_type_optional.value());
  }

  return true;
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
    opt_action_type = static_cast<CategoryContentOptActionType>(
        document["opt_action"].GetInt());
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const CategoryContentInfo& info) {
  writer->StartObject();

  writer->String("category");
  writer->String(info.category.c_str());

  writer->String("opt_action");
  writer->Int(static_cast<int>(info.opt_action_type));

  writer->EndObject();
}

}  // namespace ads
