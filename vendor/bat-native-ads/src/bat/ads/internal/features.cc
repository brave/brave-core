/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features.h"

#include <vector>

#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace features {

namespace {
const int kDefaultPageProbabilityHistorySize = 5;
const char kStudyName[] = "PageProbabilitiesHistoryStudy";
}  // namespace

// TODO(Moritz Haller): Rename feature in seed
const base::Feature kPageClassifier { "ContextualAdsControl",
    base::FEATURE_ENABLED_BY_DEFAULT };

bool IsPageClassifierEnabled() {
  return base::FeatureList::IsEnabled(kPageClassifier);
}

int GetPageProbabilitiesHistorySize() {
  return GetFieldTrialParamByFeatureAsInt(kPageClassifier,
      "page_probabilities_history_size", kDefaultPageProbabilityHistorySize);
}

const base::Feature kBanditClassifier { "BanditClassifier",
    base::FEATURE_DISABLED_BY_DEFAULT };

bool IsBanditClassifierEnabled() {
  return base::FeatureList::IsEnabled(kBanditClassifier);
}

bool HasActiveStudy() {
  if (!base::FieldTrialList::Find(kStudyName)) {
    return false;
  }

  return true;
}

std::string GetStudy() {
  return kStudyName;
}

std::string GetGroup() {
  std::string group_name;
  base::FieldTrial* field_trial =
      base::FieldTrialList::Find(kStudyName);
  if (!field_trial) {
    return group_name;
  }

  group_name = field_trial->group_name();
  return group_name;
}

void Log() {
  if (!HasActiveStudy()) {
    BLOG(1, "No active study found");
  } else {
    const std::string study = GetStudy();
    const std::string group = GetGroup();
    BLOG(1, "Running study " << study << " with group " << group);
  }

  if (IsBanditClassifierEnabled()) {
    BLOG(1, "Bandit Classifier enabled");
  } else {
    BLOG(1, "Bandit Classifier disabled");
  }

  if (IsPageClassifierEnabled()) {
    int history_size = GetPageProbabilitiesHistorySize();
    BLOG(1, "Page Classifier enabled with history size of " << history_size);
  } else {
    BLOG(1, "Page Classifier disabled");
  }
}

}  // namespace features
}  // namespace ads
