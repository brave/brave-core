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
const char kActiveStudyName[] = "PageProbabilitiesHistoryStudy";
const int kDefaultPageProbabilityHistorySize = 5;
}  // namespace

// Controls behavior of the contextual ad matching mechanism, e.g. by adjusting
// the number of text classifications used to infer user interest

// TODO use cli args to disable/enable features for QA

// TODO(Moritz Haller): create issue - remove page clf feature and exp.
// rename to text clf if we keep around
const base::Feature kContextualAdsControl { "ContextualAdsControl",
    base::FEATURE_DISABLED_BY_DEFAULT };

const base::Feature kTextClassificationModel { "TextClassificationModel",
    // base::FEATURE_ENABLED_BY_DEFAULT };
    base::FEATURE_DISABLED_BY_DEFAULT };

// TODO(Moritz Haller): remove model in feature names
bool IsTextClassificationModelEnabled() {
  return base::FeatureList::IsEnabled(kTextClassificationModel);
}

int GetPageProbabilitiesHistorySize() {
  return GetFieldTrialParamByFeatureAsInt(kTextClassificationModel,
      "page_probabilities_history_size", kDefaultPageProbabilityHistorySize);
}

// TODO(Moritz Haller): formatting
// TODO: spilit out in diff feature files
// TODO: unit test features
const base::Feature kPurchaseIntentModel {
  "PurchaseIntentModel",
  base::FEATURE_ENABLED_BY_DEFAULT
};

bool IsPurchaseIntentModelEnabled() {
  return base::FeatureList::IsEnabled(kPurchaseIntentModel);
}

const base::Feature kEpsilonGreedyBandit { "EpsilonGreedyBanditModel",
    // base::FEATURE_DISABLED_BY_DEFAULT };
    base::FEATURE_ENABLED_BY_DEFAULT };

bool IsEpsilonGreedyBanditEnabled() {
  return base::FeatureList::IsEnabled(kEpsilonGreedyBandit);
}

bool HasActiveStudy() {
  if (!base::FieldTrialList::Find(kActiveStudyName)) {
    return false;
  }

  return true;
}

std::string GetStudy() {
  return kActiveStudyName;
}

std::string GetGroup() {
  std::string group_name;
  base::FieldTrial* field_trial = base::FieldTrialList::Find(kActiveStudyName);
  if (!field_trial) {
    return group_name;
  }

  group_name = field_trial->group_name();
  return group_name;
}

// TODO(Moritz Haller): pull out into |features_logging.cc|
// ads::features::Log();
void Log() {
  if (!HasActiveStudy()) {
    BLOG(1, "No active study found");
  } else {
    const std::string study = GetStudy();
    const std::string group = GetGroup();
    BLOG(1, "Running study " << study << " with group " << group);
  }

  if (IsTextClassificationModelEnabled()) {
    BLOG(1, "Text Classification Model enabled");
  } else {
    BLOG(1, "Text Classification Model disabled");
  }

  if (IsPurchaseIntentModelEnabled()) {
    BLOG(1, "Purchase Intent Model enabled");
  } else {
    BLOG(1, "Purchase Intent Model disabled");
  }

  if (IsEpsilonGreedyBanditEnabled()) {
    BLOG(1, "Epsilon Greedy Bandit enabled");
  } else {
    BLOG(1, "Epsilon Greedy Bandit disabled");
  }
}

}  // namespace features
}  // namespace ads
