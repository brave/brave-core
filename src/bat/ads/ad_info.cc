/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_info.h"

#include "json_helper.h"

namespace ads {

AdInfo::AdInfo() :
    creative_set_id(""),
    start_timestamp(""),
    end_timestamp(""),
    regions({}),
    advertiser(""),
    notification_text(""),
    notification_url(""),
    uuid("") {}

AdInfo::AdInfo(const AdInfo& info) :
    creative_set_id(info.creative_set_id),
    start_timestamp(info.start_timestamp),
    end_timestamp(info.end_timestamp),
    regions(info.regions),
    advertiser(info.advertiser),
    notification_text(info.notification_text),
    notification_url(info.notification_url),
    uuid(info.uuid) {}

AdInfo::~AdInfo() = default;

const std::string AdInfo::ToJson() {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool AdInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    return false;
  }

  if (document.HasMember("creative_set_id")) {
    creative_set_id = document["creative_set_id"].GetString();
  }

  if (document.HasMember("start_timestamp")) {
    start_timestamp = document["start_timestamp"].GetString();
  }

  if (document.HasMember("end_timestamp")) {
    end_timestamp = document["end_timestamp"].GetString();
  }

  std::vector<std::string> regions = {};
  if (document.HasMember("regions")) {
    for (const auto& region : document["regions"].GetArray()) {
      regions.push_back(region.GetString());
    }
  }

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

  return true;
}

void SaveToJson(JsonWriter* writer, const AdInfo& info) {
  writer->StartObject();

  writer->String("creative_set_id");
  writer->String(info.creative_set_id.c_str());

  writer->String("start_timestamp");
  writer->String(info.start_timestamp.c_str());

  writer->String("end_timestamp");
  writer->String(info.end_timestamp.c_str());

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
