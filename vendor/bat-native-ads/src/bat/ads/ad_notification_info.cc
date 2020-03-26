/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/json_helper.h"
#include "base/logging.h"

namespace ads {

AdNotificationInfo::AdNotificationInfo() = default;

AdNotificationInfo::AdNotificationInfo(
    const AdNotificationInfo& info) = default;

AdNotificationInfo::~AdNotificationInfo() = default;

std::string AdNotificationInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result AdNotificationInfo::FromJson(
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

  if (document.HasMember("id")) {
    uuid = document["id"].GetString();
  }

  if (document.HasMember("parent_id")) {
    parent_uuid = document["parent_id"].GetString();
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

  if (document.HasMember("advertiser")) {
    title = document["advertiser"].GetString();
  }

  if (document.HasMember("text")) {
    body = document["text"].GetString();
  }

  if (document.HasMember("url")) {
    target_url = document["url"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(
    JsonWriter* writer,
    const AdNotificationInfo& info) {
  writer->StartObject();

  writer->String("id");
  writer->String(info.uuid.c_str());

  writer->String("parent_uuid");
  writer->String(info.parent_uuid.c_str());

  writer->String("uuid");
  writer->String(info.creative_instance_id.c_str());

  writer->String("creative_set_id");
  writer->String(info.creative_set_id.c_str());

  writer->String("category");
  writer->String(info.category.c_str());

  writer->String("advertiser");
  writer->String(info.title.c_str());

  writer->String("text");
  writer->String(info.body.c_str());

  writer->String("url");
  writer->String(info.target_url.c_str());

  writer->String("geo_target");
  writer->String(info.geo_target.c_str());

  writer->EndObject();
}

}  // namespace ads
