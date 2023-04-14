/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_embedding_exclusion_rule.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_features.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

namespace brave_ads::notification_ads {

namespace {

constexpr int kCompatibleServingVersion = 3;

bool DoesRespectCap(const CreativeAdInfo& creative_ad) {
  if (notification_ads::kServingVersion.Get() != kCompatibleServingVersion) {
    return true;
  }

  return !creative_ad.embedding.empty();
}

}  // namespace

std::string EmbeddingExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool EmbeddingExclusionRule::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s does not have a matching embedding",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

const std::string& EmbeddingExclusionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace brave_ads::notification_ads
