/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/notification_ad_info.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "url/gurl.h"

namespace ads {

NotificationAdInfo::NotificationAdInfo() = default;

NotificationAdInfo::NotificationAdInfo(const NotificationAdInfo& info) =
    default;

NotificationAdInfo& NotificationAdInfo::operator=(
    const NotificationAdInfo& info) = default;

NotificationAdInfo::~NotificationAdInfo() = default;

bool NotificationAdInfo::IsValid() const {
  if (!AdInfo::IsValid()) {
    return false;
  }

  if (title.empty() || body.empty()) {
    return false;
  }

  return true;
}

base::Value::Dict NotificationAdInfo::ToValue() const {
  base::Value::Dict dict;
  dict.Set("type", type.ToString());
  dict.Set("uuid", placement_id);
  dict.Set("creative_instance_id", creative_instance_id);
  dict.Set("creative_set_id", creative_set_id);
  dict.Set("campaign_id", campaign_id);
  dict.Set("advertiser_id", advertiser_id);
  dict.Set("segment", segment);
  dict.Set("title", title);
  dict.Set("body", body);
  dict.Set("target_url", target_url.spec());
  return dict;
}

void NotificationAdInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindString("type")) {
    type = AdType(*value);
  }

  if (const auto* value = root.FindString("uuid")) {
    placement_id = *value;
  }

  if (const auto* value = root.FindString("creative_instance_id")) {
    creative_instance_id = *value;
  }

  if (const auto* value = root.FindString("creative_set_id")) {
    creative_set_id = *value;
  }

  if (const auto* value = root.FindString("campaign_id")) {
    campaign_id = *value;
  }

  if (const auto* value = root.FindString("advertiser_id")) {
    advertiser_id = *value;
  }

  if (const auto* value = root.FindString("segment")) {
    segment = *value;
  }

  if (const auto* value = root.FindString("title")) {
    title = *value;
  }

  if (const auto* value = root.FindString("body")) {
    body = *value;
  }

  if (const auto* value = root.FindString("target_url")) {
    target_url = GURL(*value);
  }
}

std::string NotificationAdInfo::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool NotificationAdInfo::FromJson(const std::string& json) {
  absl::optional<base::Value> document =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);

  if (!document.has_value() || !document->is_dict()) {
    return false;
  }

  FromValue(document->GetDict());

  return true;
}

}  // namespace ads
