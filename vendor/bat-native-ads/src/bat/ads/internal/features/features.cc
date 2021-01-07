/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/features.h"

#include "bat/ads/internal/features/bandits/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/features/purchase_intent/purchase_intent_features.h"
#include "bat/ads/internal/features/text_classification/text_classification_features.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace features {

namespace {
const char kActiveStudyName[] = "EpsilonGreedyBanditStudy";
}  // namespace

bool HasActiveStudy() {
  if (!base::FieldTrialList::Find(kActiveStudyName)) {
    return false;
  }

  return true;
}

base::Optional<std::string> GetStudy() {
  std::string study_name(kActiveStudyName);
  if (study_name.empty()) {
    return base::nullopt;
  }

  return study_name;
}

base::Optional<std::string> GetGroup() {
  base::FieldTrial* field_trial =
      base::FieldTrialList::Find(kActiveStudyName);
  if (!field_trial) {
    return base::nullopt;
  }

  return field_trial->group_name();
}

void Log() {
  const base::Optional<std::string> study = GetStudy();
  const base::Optional<std::string> group = GetGroup();
  if (HasActiveStudy() && study.has_value() && group.has_value()) {
    BLOG(1, "Active study " << study.value() << " in group " << group.value());
  } else {
    BLOG(1, "No active study found");
  }

  BLOG(1, "Text classification feature is "
      << (IsTextClassificationEnabled() ? "enabled" : "disabled"));
  BLOG(1, "Epsilon greedy bandit feature is "
      << (IsEpsilonGreedyBanditEnabled() ? "enabled" : "disabled"));
  BLOG(1, "Purchase intent feature is "
      << (IsPurchaseIntentEnabled() ? "enabled" : "disabled"));
}

}  // namespace features
}  // namespace ads
