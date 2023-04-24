/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_processor.h"

#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/containers/contains.h"
#include "base/notreached.h"
#include "base/strings/string_piece.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_arm_info.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_arm_util.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_arms_alias.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feedback_info.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_segments.h"
#include "brave/components/brave_ads/core/internal/segments/segment_util.h"

namespace brave_ads {

namespace {

constexpr double kDefaultArmValue = 1.0;
constexpr int kDefaultArmPulls = 0;

void MaybeAddOrResetArms(EpsilonGreedyBanditArmMap* arms) {
  DCHECK(arms);

  for (const base::StringPiece value : GetSegments()) {
    std::string segment = static_cast<std::string>(value);
    const auto iter = arms->find(segment);
    if (iter != arms->cend()) {
      const EpsilonGreedyBanditArmInfo arm = iter->second;
      if (arm.IsValid()) {
        BLOG(3, "Epsilon greedy bandit arm already exists for " << segment
                                                                << " segment");

        continue;
      }
    }

    EpsilonGreedyBanditArmInfo arm;
    arm.value = kDefaultArmValue;
    arm.pulls = kDefaultArmPulls;

    arms->insert_or_assign(std::move(segment), arm);

    BLOG(2,
         "Epsilon greedy bandit arm was added for " << segment << " segment");
  }
}

void MaybeDeleteArms(EpsilonGreedyBanditArmMap* arms) {
  DCHECK(arms);

  for (auto iter = arms->cbegin(); iter != arms->cend();) {
    if (base::Contains(GetSegments(), iter->first)) {
      ++iter;
      continue;
    }

    BLOG(2, "Epsilon greedy bandit arm was deleted for " << iter->first
                                                         << " segment ");

    iter = arms->erase(iter);
  }
}

void InitializeArms() {
  EpsilonGreedyBanditArmMap arms = GetEpsilonGreedyBanditArms();

  MaybeAddOrResetArms(&arms);

  MaybeDeleteArms(&arms);

  SetEpsilonGreedyBanditArms(arms);

  BLOG(1, "Successfully initialized epsilon greedy bandit arms");
}

void UpdateArm(const int reward, const std::string& segment) {
  EpsilonGreedyBanditArmMap arms = GetEpsilonGreedyBanditArms();
  if (arms.empty()) {
    BLOG(1, "No epsilon greedy bandit arms");
    return;
  }

  const auto iter = arms.find(segment);
  if (iter == arms.cend()) {
    BLOG(1, "Epsilon greedy bandit arm was not found for " << segment
                                                           << " segment");
    return;
  }

  EpsilonGreedyBanditArmInfo arm = iter->second;
  arm.pulls++;
  DCHECK_NE(0, arm.pulls);
  arm.value =
      arm.value + (1.0 / arm.pulls * (static_cast<double>(reward) - arm.value));
  iter->second = arm;

  SetEpsilonGreedyBanditArms(arms);

  BLOG(1,
       "Epsilon greedy bandit arm was updated for " << segment << " segment");
}

}  // namespace

EpsilonGreedyBanditProcessor::EpsilonGreedyBanditProcessor() {
  InitializeArms();
}

// static
void EpsilonGreedyBanditProcessor::Process(
    const EpsilonGreedyBanditFeedbackInfo& feedback) {
  DCHECK(!feedback.segment.empty());

  const std::string segment = GetParentSegment(feedback.segment);
  DCHECK(!segment.empty());

  const mojom::NotificationAdEventType ad_event_type = feedback.ad_event_type;
  DCHECK(mojom::IsKnownEnumValue(ad_event_type));
  switch (ad_event_type) {
    case mojom::NotificationAdEventType::kTimedOut:
    case mojom::NotificationAdEventType::kDismissed: {
      UpdateArm(/*reward*/ 0, segment);
      break;
    }

    case mojom::NotificationAdEventType::kClicked: {
      UpdateArm(/*reward*/ 1, segment);
      break;
    }

    case mojom::NotificationAdEventType::kServed:
    case mojom::NotificationAdEventType::kViewed: {
      NOTREACHED();
      break;
    }
  }

  BLOG(1, "Epsilon greedy bandit processed " << feedback.ad_event_type);
}

}  // namespace brave_ads
