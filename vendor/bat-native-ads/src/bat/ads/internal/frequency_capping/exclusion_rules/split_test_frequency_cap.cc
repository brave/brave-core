/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/split_test_frequency_cap.h"

#include "base/metrics/field_trial.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

namespace {

const char kStudyName[] = "AdvertiserSplitTestStudy";

absl::optional<std::string> GetSplitTestGroup(const std::string study_name) {
  base::FieldTrial* field_trial = base::FieldTrialList::Find(study_name);
  if (!field_trial) {
    return absl::nullopt;
  }

  return field_trial->group_name();
}

}  // namespace

SplitTestFrequencyCap::SplitTestFrequencyCap() = default;

SplitTestFrequencyCap::~SplitTestFrequencyCap() = default;

bool SplitTestFrequencyCap::ShouldExclude(const CreativeAdInfo& ad) {
  if (!DoesRespectCap(ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded as not "
        "associated with advertiser split test group",
        ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string SplitTestFrequencyCap::get_last_message() const {
  return last_message_;
}

bool SplitTestFrequencyCap::DoesRespectCap(const CreativeAdInfo& ad) const {
  const absl::optional<std::string> split_test_group =
      GetSplitTestGroup(kStudyName);
  if (!split_test_group) {
    // Only respect cap if browser has signed up to a field trial
    return ad.split_test_group.empty();
  }

  if (ad.split_test_group.empty()) {
    // Always respect cap if there is no split testing group in the catalog
    return true;
  }

  if (ad.split_test_group == split_test_group) {
    return true;
  }

  return false;
}

}  // namespace ads
