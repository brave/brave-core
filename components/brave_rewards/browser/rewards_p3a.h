/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace brave_rewards {
namespace p3a {

inline constexpr char kEnabledSourceHistogramName[] =
    "Brave.Rewards.EnabledSource";
inline constexpr char kToolbarButtonTriggerHistogramName[] =
    "Brave.Rewards.ToolbarButtonTrigger";
inline constexpr char kTipsSentHistogramName[] = "Brave.Rewards.TipsSent.2";
inline constexpr char kAutoContributionsStateHistogramName[] =
    "Brave.Rewards.AutoContributionsState.3";
inline constexpr char kAdTypesEnabledHistogramName[] =
    "Brave.Rewards.AdTypesEnabled";
inline constexpr char kMobileConversionHistogramName[] =
    "Brave.Rewards.MobileConversion";
inline constexpr char kMobilePanelCountHistogramName[] =
    "Brave.Rewards.MobilePanelCount";

enum class AutoContributionsState {
  kNoWallet,
  kRewardsDisabled,
  kWalletCreatedAutoContributeOff,
  kAutoContributeOn,
};

enum class PanelTrigger {
  kInlineTip = 0,  // DEPRECATED
  kToolbarButton = 1,
  kNTP = 2,
  kMaxValue = kNTP
};

void RecordTipsSent(size_t tip_count);

void RecordAutoContributionsState(bool ac_enabled);

void RecordNoWalletCreatedForAllMetrics();

enum class AdTypesEnabled {
  kNone,
  kNTP,
  kNotification,
  kAll,
  kMaxValue = kAll,
};

void RecordAdTypesEnabled(PrefService* prefs);

class ConversionMonitor {
 public:
  explicit ConversionMonitor(PrefService* prefs);
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
#if BUILDFLAG(IS_ANDROID)
  void ReportPeriodicMetrics();
  void OnMobileTriggerTimer();
  void ReportMobilePanelTriggerCount();

  raw_ptr<PrefService> prefs_;
  WeeklyStorage mobile_panel_trigger_count_;
  base::OneShotTimer mobile_trigger_timer_;
  base::WallClockTimer daily_timer_;
#else
  absl::optional<PanelTrigger> last_trigger_;
  base::Time last_trigger_time_;
#endif
};

}  // namespace p3a
}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_P3A_H_
