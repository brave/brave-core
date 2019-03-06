/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/notification_info.h"

#include "bat/ads/internal/json_helper.h"

namespace ads {

NotificationInfo::NotificationInfo() :
    creative_set_id(""),
    category(""),
    advertiser(""),
    text(""),
    url(""),
    uuid("") {}

NotificationInfo::NotificationInfo(const NotificationInfo& info) :
    creative_set_id(info.creative_set_id),
    category(info.category),
    advertiser(info.advertiser),
    text(info.text),
    url(info.url),
    uuid(info.uuid) {}

NotificationInfo::~NotificationInfo() = default;

const std::string NotificationInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result NotificationInfo::FromJson(
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

  if (document.HasMember("creative_set_id")) {
    creative_set_id = document["creative_set_id"].GetString();
  }

  if (document.HasMember("category")) {
    category = document["category"].GetString();
  }

  if (document.HasMember("advertiser")) {
    advertiser = document["advertiser"].GetString();
  }

  if (document.HasMember("text")) {
    text = document["text"].GetString();
  }

  if (document.HasMember("url")) {
    url = document["url"].GetString();
  }

  if (document.HasMember("uuid")) {
    uuid = document["uuid"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const NotificationInfo& info) {
  writer->StartObject();

  writer->String("creative_set_id");
  writer->String(info.creative_set_id.c_str());

  writer->String("category");
  writer->String(info.category.c_str());

  writer->String("advertiser");
  writer->String(info.advertiser.c_str());

  writer->String("text");
  writer->String(info.text.c_str());

  writer->String("url");
  writer->String(info.url.c_str());

  writer->String("uuid");
  writer->String(info.uuid.c_str());

  writer->EndObject();
}

}  // namespace ads
