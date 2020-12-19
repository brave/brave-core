/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/features.h"

#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace features {

namespace {

const char kPageProbabilitiesStudyName[] = "PageProbabilitiesHistoryStudy";

const int kDefaultHistorySize = 5;

}  // namespace

// Controls behavior of the contextual ad matching mechanism, e.g. by adjusting
// the number of text classifications used to infer user interest
const base::Feature kContextualAdsControl { "ContextualAdsControl",
    base::FEATURE_DISABLED_BY_DEFAULT };

bool IsPageProbabilitiesStudyActive() {
  if (!base::FieldTrialList::Find(kPageProbabilitiesStudyName)) {
    return false;
  }

  return true;
}

std::string GetPageProbabilitiesStudy() {
  return kPageProbabilitiesStudyName;
}

std::string GetPageProbabilitiesFieldTrialGroup() {
  std::string group_name;
  base::FieldTrial* field_trial = base::FieldTrialList::Find(
      kPageProbabilitiesStudyName);
  if (!field_trial) {
    return group_name;
  }

  group_name = field_trial->group_name();
  return group_name;
}

int GetPageProbabilitiesHistorySize() {
  return GetFieldTrialParamByFeatureAsInt(
      kContextualAdsControl,
      "page_probabilities_history_size",
      kDefaultHistorySize);
}

void LogPageProbabilitiesStudy() {
  std::string log;
  if (!IsPageProbabilitiesStudyActive()) {
    BLOG(1, "No active experiment");
  } else {
    const std::string study = GetPageProbabilitiesStudy();
    const std::string group = GetPageProbabilitiesFieldTrialGroup();
    const std::string history_size =
        base::NumberToString(GetPageProbabilitiesHistorySize());

    BLOG(1, "Running active experiment:\n"
      << "  name: " << study << "\n"
      << "  group: " << group << "\n"
      << "  value: " << history_size);
  }
}

}  // namespace features
}  // namespace ads
