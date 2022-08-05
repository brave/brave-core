/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/promoted_content_ad_info.h"

namespace ads {

PromotedContentAdInfo::PromotedContentAdInfo() = default;

PromotedContentAdInfo::PromotedContentAdInfo(
    const PromotedContentAdInfo& info) = default;

PromotedContentAdInfo& PromotedContentAdInfo::operator=(
    const PromotedContentAdInfo& info) = default;

PromotedContentAdInfo::~PromotedContentAdInfo() = default;

base::Value::Dict PromotedContentAdInfo::ToValue() const {
  base::Value::Dict dict;
  dict.Set("type", type.ToString());
  dict.Set("uuid", placement_id);
  dict.Set("creative_instance_id", creative_instance_id);
  dict.Set("creative_set_id", creative_set_id);
  dict.Set("campaign_id", campaign_id);
  dict.Set("advertiser_id", advertiser_id);
  dict.Set("segment", segment);
  dict.Set("title", title);
  dict.Set("description", description);
  dict.Set("target_url", target_url.spec());
  return dict;
}

bool PromotedContentAdInfo::FromValue(const base::Value::Dict& root) {
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

  if (const auto* value = root.FindString("description")) {
    description = *value;
  }

  if (const auto* value = root.FindString("target_url")) {
    target_url = GURL(*value);
  }

  return true;
}

bool PromotedContentAdInfo::IsValid() const {
  if (!AdInfo::IsValid()) {
    return false;
  }

  if (title.empty() || description.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads
