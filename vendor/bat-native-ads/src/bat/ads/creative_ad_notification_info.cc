/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/creative_ad_notification_info.h"
#include "bat/ads/internal/json_helper.h"

namespace ads {

CreativeAdNotificationInfo::CreativeAdNotificationInfo() = default;

CreativeAdNotificationInfo::CreativeAdNotificationInfo(
    const CreativeAdNotificationInfo& info) = default;

CreativeAdNotificationInfo::~CreativeAdNotificationInfo() = default;

const std::string CreativeAdNotificationInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result CreativeAdNotificationInfo::FromJson(
    const std::string& json,
    std::string* error_description) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    if (error_description) {
      *error_description = helper::JSON::GetLastError(&document);
    }

    return FAILED;
  }

  if (document.HasMember("creative_set_id")) {
    creative_set_id = document["creative_set_id"].GetString();
  }

  if (document.HasMember("campaign_id")) {
    campaign_id = document["campaign_id"].GetString();
  }

  if (document.HasMember("start_at_timestamp")) {
    start_at_timestamp = document["start_at_timestamp"].GetString();
  }

  if (document.HasMember("end_at_timestamp")) {
    end_at_timestamp = document["end_at_timestamp"].GetString();
  }

  if (document.HasMember("daily_cap")) {
    daily_cap = document["daily_cap"].GetUint();
  }

  if (document.HasMember("per_day")) {
    per_day = document["per_day"].GetUint();
  }

  if (document.HasMember("total_max")) {
    total_max = document["total_max"].GetUint();
  }

  if (document.HasMember("category")) {
    category = document["category"].GetString();
  }

  std::vector<std::string> new_geo_targets;
  if (document.HasMember("geo_targets")) {
    for (const auto& geo_target : document["geo_targets"].GetArray()) {
      geo_targets.push_back(geo_target.GetString());
    }
  }
  geo_targets = new_geo_targets;

  if (document.HasMember("title")) {
    title = document["title"].GetString();
  }

  if (document.HasMember("body")) {
    body = document["body"].GetString();
  }

  if (document.HasMember("target_url")) {
    target_url = document["target_url"].GetString();
  }

  if (document.HasMember("creative_instance_id")) {
    creative_instance_id = document["creative_instance_id"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(
    JsonWriter* writer,
    const CreativeAdNotificationInfo& info) {
  writer->StartObject();

  writer->String("creative_set_id");
  writer->String(info.creative_set_id.c_str());

  writer->String("campaign_id");
  writer->String(info.campaign_id.c_str());

  writer->String("start_at_timestamp");
  writer->String(info.start_at_timestamp.c_str());

  writer->String("end_at_timestamp");
  writer->String(info.end_at_timestamp.c_str());

  writer->String("daily_cap");
  writer->Uint(info.daily_cap);

  writer->String("per_day");
  writer->Uint(info.per_day);

  writer->String("total_max");
  writer->Uint(info.total_max);

  writer->String("category");
  writer->String(info.category.c_str());

  writer->String("geo_targets");
  writer->StartArray();
  for (const auto& geo_target : info.geo_targets) {
    writer->String(geo_target.c_str());
  }
  writer->EndArray();

  writer->String("title");
  writer->String(info.title.c_str());

  writer->String("body");
  writer->String(info.body.c_str());

  writer->String("target_url");
  writer->String(info.target_url.c_str());

  writer->String("creative_instance_id");
  writer->String(info.creative_instance_id.c_str());

  writer->EndObject();
}

}  // namespace ads
