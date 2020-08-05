/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/subdivision_targeting_frequency_cap.h"

#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/locale/subdivision_code_util.h"
#include "bat/ads/internal/server/get_subdivision/subdivision_targeting.h"

namespace ads {

SubdivisionTargetingFrequencyCap::SubdivisionTargetingFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

SubdivisionTargetingFrequencyCap::~SubdivisionTargetingFrequencyCap() = default;

bool SubdivisionTargetingFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!DoesRespectCap(ad)) {
    last_message_ = base::StringPrintf("creativeSetId %s excluded as not "
        "within the targeted subdivision", ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string SubdivisionTargetingFrequencyCap::get_last_message() const {
  return last_message_;
}

bool SubdivisionTargetingFrequencyCap::DoesRespectCap(
    const CreativeAdInfo& ad) const {
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();

  if (!ads_->get_subdivision_targeting()->ShouldAllowForLocale(locale)) {
    return !DoesAdTargetSubdivision(ad);
  }

  if (ads_->get_subdivision_targeting()->IsDisabled()) {
    return !DoesAdTargetSubdivision(ad);
  }

  const std::string subdivision_targeting_code =
      ads_->get_subdivision_targeting()->GetAdsSubdivisionTargetingCode();

  return DoesAdSupportSubdivisionTargetingCode(ad, subdivision_targeting_code);
}

bool SubdivisionTargetingFrequencyCap::DoesAdSupportSubdivisionTargetingCode(
    const CreativeAdInfo& ad,
    const std::string& subdivision_targeting_code) const {
  const std::string country_code =
      locale::GetCountryCode(subdivision_targeting_code);

  const auto iter = std::find_if(ad.geo_targets.begin(), ad.geo_targets.end(),
      [&](const std::string& geo_target) {
    return geo_target == subdivision_targeting_code ||
        geo_target == country_code;
  });

  if (iter == ad.geo_targets.end()) {
    return false;
  }

  return true;
}

bool SubdivisionTargetingFrequencyCap::DoesAdTargetSubdivision(
    const CreativeAdInfo& ad) const {
  const auto iter = std::find_if(ad.geo_targets.begin(), ad.geo_targets.end(),
      [&](const std::string& geo_target) {
    const std::vector<std::string> components = base::SplitString(
        geo_target, "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

    return components.size() == 2;
  });

  if (iter == ad.geo_targets.end()) {
    return false;
  }

  return true;
}

}  // namespace ads
