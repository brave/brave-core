/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/inline_content_ad_info.h"

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ads/confirmation_type.h"

namespace ads {

InlineContentAdInfo::InlineContentAdInfo() = default;

InlineContentAdInfo::InlineContentAdInfo(const InlineContentAdInfo& info) =
    default;

InlineContentAdInfo& InlineContentAdInfo::operator=(
    const InlineContentAdInfo& info) = default;

InlineContentAdInfo::~InlineContentAdInfo() = default;

bool InlineContentAdInfo::operator==(const InlineContentAdInfo& rhs) const {
  return AdInfo::operator==(rhs) && title == rhs.title &&
         description == rhs.description && image_url == rhs.image_url &&
         dimensions == rhs.dimensions && cta_text == rhs.cta_text;
}

bool InlineContentAdInfo::operator!=(const InlineContentAdInfo& rhs) const {
  return !(*this == rhs);
}

bool InlineContentAdInfo::IsValid() const {
  if (!AdInfo::IsValid()) {
    return false;
  }

  if (title.empty() || description.empty() || !image_url.is_valid() ||
      dimensions.empty() || cta_text.empty()) {
    return false;
  }

  return true;
}

base::Value::Dict InlineContentAdInfo::ToValue() const {
  base::Value::Dict dict;

  dict.Set("type", static_cast<int>(type.value()));
  dict.Set("uuid", placement_id);
  dict.Set("creativeInstanceId", creative_instance_id);
  dict.Set("creativeSetId", creative_set_id);
  dict.Set("campaignId", campaign_id);
  dict.Set("advertiserId", advertiser_id);
  dict.Set("segment", segment);
  dict.Set("title", title);
  dict.Set("description", description);
  dict.Set("imageUrl", image_url.spec());
  dict.Set("dimensions", dimensions);
  dict.Set("ctaText", cta_text);
  dict.Set("targetUrl", target_url.spec());

  return dict;
}

void InlineContentAdInfo::FromValue(const base::Value::Dict& root) {
  if (const auto* value = root.FindString("type")) {
    type = AdType(*value);
  }

  if (const auto* value = root.FindString("uuid")) {
    placement_id = *value;
  }

  if (const auto* value = root.FindString("creativeInstanceId")) {
    creative_instance_id = *value;
  }

  if (const auto* value = root.FindString("creativeSetId")) {
    creative_set_id = *value;
  }

  if (const auto* value = root.FindString("campaignId")) {
    campaign_id = *value;
  }

  if (const auto* value = root.FindString("advertiserId")) {
    advertiser_id = *value;
  }

  if (const auto* value = root.FindString("segment")) {
    segment = *value;
  }

  if (const auto* value = root.FindString("title")) {
    title = *value;
  }

  if (const auto* value = root.FindString("description")) {
    description = *value;
  }

  if (const auto* value = root.FindString("imageUrl")) {
    image_url = GURL(*value);
  }

  if (const auto* value = root.FindString("dimensions")) {
    dimensions = *value;
  }

  if (const auto* value = root.FindString("ctaText")) {
    cta_text = *value;
  }

  if (const auto* value = root.FindString("targetUrl")) {
    target_url = GURL(*value);
  }
}

std::string InlineContentAdInfo::ToJson() const {
  std::string json;
  CHECK(base::JSONWriter::Write(ToValue(), &json));
  return json;
}

bool InlineContentAdInfo::FromJson(const std::string& json) {
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
