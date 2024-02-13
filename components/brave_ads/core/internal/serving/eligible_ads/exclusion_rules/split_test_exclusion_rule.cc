/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/split_test_exclusion_rule.h"

#include <optional>

#include "base/metrics/field_trial.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

namespace brave_ads {

namespace {

constexpr char kTrialName[] = "AdvertiserSplitTestStudy";

std::optional<std::string> GetSplitTestGroupName(
    const std::string& trial_name) {
  base::FieldTrial* field_trial = base::FieldTrialList::Find(trial_name);
  if (!field_trial) {
    return std::nullopt;
  }

  return field_trial->group_name();
}

bool DoesRespectCap(const CreativeAdInfo& creative_ad) {
  const std::optional<std::string> split_test_group =
      GetSplitTestGroupName(kTrialName);
  if (!split_test_group) {
    // Only respect cap if browser has signed up to a field trial.
    return creative_ad.split_test_group.empty();
  }

  if (creative_ad.split_test_group.empty()) {
    // Always respect cap if there is no split testing group in the catalog.
    return true;
  }

  if (creative_ad.split_test_group == split_test_group) {
    return true;
  }

  return false;
}

}  // namespace

std::string SplitTestExclusionRule::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.creative_set_id;
}

base::expected<void, std::string> SplitTestExclusionRule::ShouldInclude(
    const CreativeAdInfo& creative_ad) const {
  if (!DoesRespectCap(creative_ad)) {
    return base::unexpected(base::ReplaceStringPlaceholders(
        "creativeSetId $1 excluded as not associated with an advertiser split "
        "test group",
        {creative_ad.creative_set_id}, nullptr));
  }

  return base::ok();
}

}  // namespace brave_ads
