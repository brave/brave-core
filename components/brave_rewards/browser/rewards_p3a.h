/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_

#include "base/time/time.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace brave_rewards {
namespace p3a {

extern const char kEnabledSourceHistogramName[];
extern const char kInlineTipTriggerHistogramName[];
extern const char kToolbarButtonTriggerHistogramName[];
extern const char kTipsSentHistogramName[];
extern const char kAutoContributionsStateHistogramName[];
extern const char kAdsEnabledDurationHistogramName[];

enum class AutoContributionsState {
  kNoWallet,
  kRewardsDisabled,
  kWalletCreatedAutoContributeOff,
  kAutoContributeOn,
};

enum class PanelTrigger {
  kInlineTip = 0,
  kToolbarButton = 1,
  kNTP = 2,
  kMaxValue = kNTP
};

enum class AdsState {
  kNoWallet,
  kRewardsDisabled,
  kAdsDisabled,
  kAdsEnabled,
  kAdsEnabledThenDisabledRewardsOn,
  kAdsEnabledThenDisabledRewardsOff,
  kMaxValue = kAdsEnabledThenDisabledRewardsOff,
};

void RecordTipsSent(size_t tip_count);

void RecordAutoContributionsState(bool ac_enabled);

void RecordNoWalletCreatedForAllMetrics();

enum class AdsEnabledDuration {
  kNever,
  kStillEnabled,
  kHours,
  kDays,
  kWeeks,
  kMonths,
  kQuarters,
  kMaxValue = kQuarters,
};

void RecordAdsEnabledDuration(PrefService* prefs, bool ads_enabled);

class ConversionMonitor {
 public:
  ConversionMonitor();
  ~ConversionMonitor();

  ConversionMonitor(const ConversionMonitor&) = delete;
  ConversionMonitor& operator=(const ConversionMonitor&) = delete;

  // Record trigger of an action that could potentially trigger opening the
  // Rewards panel. Will immediately record the action (if applicable).
  void RecordPanelTrigger(PanelTrigger trigger);

  // Record the enabled of rewards, which may record a metric containing the
  // source of user conversion a.k.a. the action that opened the Rewards panel.
  void RecordRewardsEnable();

 private:
  absl::optional<PanelTrigger> last_trigger_;
  base::Time last_trigger_time_;
};

}  // namespace p3a
}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_
