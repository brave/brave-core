/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_info.h"

#include "json_helper.h"

namespace ads {

AdInfo::AdInfo() :
    creative_set_id(""),
    campaign_id(""),
    start_timestamp(""),
    end_timestamp(""),
    daily_cap(0),
    per_day(0),
    total_max(0),
    regions({}),
    advertiser(""),
    notification_text(""),
    notification_url(""),
    uuid("") {}

AdInfo::AdInfo(const AdInfo& info) :
    creative_set_id(info.creative_set_id),
    campaign_id(info.campaign_id),
    start_timestamp(info.start_timestamp),
    end_timestamp(info.end_timestamp),
    daily_cap(info.daily_cap),
    per_day(info.per_day),
    total_max(info.total_max),
    regions(info.regions),
    advertiser(info.advertiser),
    notification_text(info.notification_text),
    notification_url(info.notification_url),
    uuid(info.uuid) {}

AdInfo::~AdInfo() = default;

const std::string AdInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

Result AdInfo::FromJson(
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

  if (document.HasMember("start_timestamp")) {
    start_timestamp = document["start_timestamp"].GetString();
  }

  if (document.HasMember("end_timestamp")) {
    end_timestamp = document["end_timestamp"].GetString();
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

  std::vector<std::string> new_regions = {};
  if (document.HasMember("regions")) {
    for (const auto& region : document["regions"].GetArray()) {
      new_regions.push_back(region.GetString());
    }
  }
  regions = new_regions;

  if (document.HasMember("advertiser")) {
    advertiser = document["advertiser"].GetString();
  }

  if (document.HasMember("notification_text")) {
    notification_text = document["notification_text"].GetString();
  }

  if (document.HasMember("notification_url")) {
    notification_url = document["notification_url"].GetString();
  }

  if (document.HasMember("uuid")) {
    uuid = document["uuid"].GetString();
  }

  return SUCCESS;
}

void SaveToJson(JsonWriter* writer, const AdInfo& info) {
  writer->StartObject();

  writer->String("creative_set_id");
  writer->String(info.creative_set_id.c_str());

  writer->String("campaign_id");
  writer->String(info.campaign_id.c_str());

  writer->String("start_timestamp");
  writer->String(info.start_timestamp.c_str());

  writer->String("end_timestamp");
  writer->String(info.end_timestamp.c_str());

  writer->String("daily_cap");
  writer->Uint(info.daily_cap);

  writer->String("per_day");
  writer->Uint(info.per_day);

  writer->String("total_max");
  writer->Uint(info.total_max);

  writer->String("regions");
  writer->StartArray();
  for (const auto& region : info.regions) {
    writer->String(region.c_str());
  }
  writer->EndArray();

  writer->String("advertiser");
  writer->String(info.advertiser.c_str());

  writer->String("notification_text");
  writer->String(info.notification_text.c_str());

  writer->String("notification_url");
  writer->String(info.notification_url.c_str());

  writer->String("uuid");
  writer->String(info.uuid.c_str());

  writer->EndObject();
}

}  // namespace ads
