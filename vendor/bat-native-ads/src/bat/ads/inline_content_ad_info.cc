/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/inline_content_ad_info.h"

#include "base/values.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/json_helper.h"
#include "bat/ads/internal/logging.h"

namespace ads {

InlineContentAdInfo::InlineContentAdInfo() = default;

InlineContentAdInfo::InlineContentAdInfo(const InlineContentAdInfo& info) =
    default;

InlineContentAdInfo::~InlineContentAdInfo() = default;

bool InlineContentAdInfo::operator==(const InlineContentAdInfo& rhs) const {
  return title == rhs.title && description == rhs.description &&
         image_url == rhs.image_url && dimensions == rhs.dimensions &&
         cta_text == rhs.cta_text;
}

bool InlineContentAdInfo::operator!=(const InlineContentAdInfo& rhs) const {
  return !(*this == rhs);
}

bool InlineContentAdInfo::IsValid() const {
  if (!AdInfo::IsValid()) {
    return false;
  }

  if (title.empty() || description.empty() || image_url.empty() ||
      dimensions.empty() || cta_text.empty()) {
    return false;
  }

  return true;
}

base::DictionaryValue InlineContentAdInfo::ToValue() const {
  base::DictionaryValue dictionary;

  dictionary.SetKey("uuid", base::Value(uuid));
  dictionary.SetKey("creativeInstanceId", base::Value(creative_instance_id));
  dictionary.SetKey("creativeSetId", base::Value(creative_set_id));
  dictionary.SetKey("campaignId", base::Value(campaign_id));
  dictionary.SetKey("advertiserId", base::Value(advertiser_id));
  dictionary.SetKey("segment", base::Value(segment));
  dictionary.SetKey("title", base::Value(title));
  dictionary.SetKey("description", base::Value(description));
  dictionary.SetKey("imageUrl", base::Value(image_url));
  dictionary.SetKey("dimensions", base::Value(dimensions));
  dictionary.SetKey("ctaText", base::Value(cta_text));
  dictionary.SetKey("targetUrl", base::Value(target_url));

  return dictionary;
}

bool InlineContentAdInfo::FromValue(const base::Value& value) {
  const base::DictionaryValue* dictionary = nullptr;
  if (!(&value)->GetAsDictionary(&dictionary)) {
    return false;
  }

  const std::string* uuid_value = dictionary->FindStringKey("uuid");
  if (uuid_value) {
    uuid = *uuid_value;
  }

  const std::string* creative_instance_id_value =
      dictionary->FindStringKey("creativeInstanceId");
  if (creative_instance_id_value) {
    creative_instance_id = *creative_instance_id_value;
  }

  const std::string* creative_set_id_value =
      dictionary->FindStringKey("creativeSetId");
  if (creative_set_id_value) {
    creative_set_id = *creative_set_id_value;
  }

  const std::string* campaign_id_value =
      dictionary->FindStringKey("campaignId");
  if (campaign_id_value) {
    campaign_id = *campaign_id_value;
  }

  const std::string* advertiser_id_value =
      dictionary->FindStringKey("advertiserId");
  if (advertiser_id_value) {
    advertiser_id = *campaign_id_value;
  }

  const std::string* segment_value = dictionary->FindStringKey("segment");
  if (segment_value) {
    segment = *segment_value;
  }

  const std::string* title_value = dictionary->FindStringKey("title");
  if (title_value) {
    title = *title_value;
  }

  const std::string* description_value =
      dictionary->FindStringKey("description");
  if (description_value) {
    description = *description_value;
  }

  const std::string* image_url_value = dictionary->FindStringKey("imageUrl");
  if (image_url_value) {
    image_url = *image_url_value;
  }

  const std::string* dimensions_value = dictionary->FindStringKey("dimensions");
  if (dimensions_value) {
    dimensions = *dimensions_value;
  }

  const std::string* cta_text_value = dictionary->FindStringKey("ctaText");
  if (cta_text_value) {
    cta_text = *cta_text_value;
  }

  const std::string* target_url_value = dictionary->FindStringKey("targetUrl");
  if (target_url_value) {
    target_url = *target_url_value;
  }

  return true;
}

std::string InlineContentAdInfo::ToJson() const {
  std::string json;
  SaveToJson(*this, &json);
  return json;
}

bool InlineContentAdInfo::FromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    BLOG(1, helper::JSON::GetLastError(&document));
    return false;
  }

  if (document.HasMember("type")) {
    type = AdType(document["type"].GetString());
  }

  if (document.HasMember("uuid")) {
    uuid = document["uuid"].GetString();
  }

  if (document.HasMember("creative_instance_id")) {
    creative_instance_id = document["creative_instance_id"].GetString();
  }

  if (document.HasMember("creative_set_id")) {
    creative_set_id = document["creative_set_id"].GetString();
  }

  if (document.HasMember("campaign_id")) {
    campaign_id = document["campaign_id"].GetString();
  }

  if (document.HasMember("advertiser_id")) {
    advertiser_id = document["advertiser_id"].GetString();
  }

  if (document.HasMember("segment")) {
    segment = document["segment"].GetString();
  }

  if (document.HasMember("title")) {
    title = document["title"].GetString();
  }

  if (document.HasMember("description")) {
    description = document["description"].GetString();
  }

  if (document.HasMember("image_url")) {
    image_url = document["image_url"].GetString();
  }

  if (document.HasMember("dimensions")) {
    dimensions = document["dimensions"].GetString();
  }

  if (document.HasMember("cta_text")) {
    cta_text = document["cta_text"].GetString();
  }

  if (document.HasMember("target_url")) {
    target_url = document["target_url"].GetString();
  }

  return true;
}

void SaveToJson(JsonWriter* writer, const InlineContentAdInfo& info) {
  writer->StartObject();

  writer->String("type");
  const std::string type = std::string(info.type);
  writer->String(type.c_str());

  writer->String("uuid");
  writer->String(info.uuid.c_str());

  writer->String("creative_instance_id");
  writer->String(info.creative_instance_id.c_str());

  writer->String("creative_set_id");
  writer->String(info.creative_set_id.c_str());

  writer->String("campaign_id");
  writer->String(info.campaign_id.c_str());

  writer->String("advertiser_id");
  writer->String(info.advertiser_id.c_str());

  writer->String("segment");
  writer->String(info.segment.c_str());

  writer->String("title");
  writer->String(info.title.c_str());

  writer->String("description");
  writer->String(info.description.c_str());

  writer->String("image_url");
  writer->String(info.image_url.c_str());

  writer->String("dimensions");
  writer->String(info.dimensions.c_str());

  writer->String("cta_text");
  writer->String(info.cta_text.c_str());

  writer->String("target_url");
  writer->String(info.target_url.c_str());

  writer->EndObject();
}

}  // namespace ads
