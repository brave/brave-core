/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/notification_ad_info.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/base/logging_util.h"

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

bool NotificationAdInfo::FromValue(const base::Value::Dict& root) {
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

  return true;
}

std::string NotificationAdInfo::ToJson() const {
  std::string json;
  base::JSONWriter::Write(ToValue(), &json);
  return json;
}

bool NotificationAdInfo::FromJson(const std::string& json) {
  auto document = base::JSONReader::ReadAndReturnValueWithError(
      json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                base::JSONParserOptions::JSON_PARSE_RFC);

  if (!document.value.has_value()) {
    BLOG(1, "Invalid notification ad info. json="
                << json << ", error line=" << document.error_line
                << ", error column=" << document.error_column
                << ", error message=" << document.error_message);
    return false;
  }

  const base::Value::Dict* root = document.value->GetIfDict();
  if (!root) {
    BLOG(1, "Invalid notifcation ad info. json=" << json);
    return false;
  }

  return FromValue(*root);
}

}  // namespace ads
