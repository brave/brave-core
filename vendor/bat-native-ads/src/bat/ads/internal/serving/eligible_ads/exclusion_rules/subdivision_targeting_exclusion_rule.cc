/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"

#include <algorithm>
#include <vector>

#include "base/check.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/base/subdivision_code_util.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/l10n/browser/locale_helper.h"

namespace ads {

namespace {

bool DoesAdSupportSubdivisionTargetingCode(
    const CreativeAdInfo& creative_ad,
    const std::string& subdivision_targeting_code) {
  const std::string country_code =
      locale::GetCountryCode(subdivision_targeting_code);

  return creative_ad.geo_targets.find(subdivision_targeting_code) !=
             creative_ad.geo_targets.end() ||
         creative_ad.geo_targets.find(country_code) !=
             creative_ad.geo_targets.end();
}

bool DoesAdTargetSubdivision(const CreativeAdInfo& creative_ad) {
  const auto iter = std::find_if(
      creative_ad.geo_targets.cbegin(), creative_ad.geo_targets.cend(),
      [](const std::string& geo_target) {
        const std::vector<std::string> components = base::SplitString(
            geo_target, "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

        return components.size() == 2;
      });

  if (iter == creative_ad.geo_targets.end()) {
    return false;
  }

  return true;
}

}  // namespace

SubdivisionTargetingExclusionRule::SubdivisionTargetingExclusionRule(
    geographic::SubdivisionTargeting* subdivision_targeting)
    : subdivision_targeting_(subdivision_targeting) {
  DCHECK(subdivision_targeting_);
}

SubdivisionTargetingExclusionRule::~SubdivisionTargetingExclusionRule() =
    default;

std::string SubdivisionTargetingExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

bool SubdivisionTargetingExclusionRule::ShouldExclude(
    const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(creative_ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded as not within the targeted subdivision",
        creative_ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string SubdivisionTargetingExclusionRule::GetLastMessage() const {
  return last_message_;
}

bool SubdivisionTargetingExclusionRule::DoesRespectCap(
    const CreativeAdInfo& creative_ad) {
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();

  if (!subdivision_targeting_->ShouldAllowForLocale(locale)) {
    return !DoesAdTargetSubdivision(creative_ad);
  }

  if (subdivision_targeting_->IsDisabled()) {
    return !DoesAdTargetSubdivision(creative_ad);
  }

  const std::string subdivision_code =
      subdivision_targeting_->GetSubdivisionCode();
  if (subdivision_code.empty()) {
    return false;
  }

  return DoesAdSupportSubdivisionTargetingCode(creative_ad, subdivision_code);
}

}  // namespace ads
