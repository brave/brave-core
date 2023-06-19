/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_PRIVACY_HUB_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_PRIVACY_HUB_METRICS_H_

#include "base/timer/wall_clock_timer.h"

#include "brave/components/misc_metrics/common/misc_metrics.mojom.h"
#include "brave/components/time_period_storage/monthly_storage.h"

#if BUILDFLAG(IS_ANDROID)
#include "mojo/public/cpp/bindings/receiver_set.h"
#endif  // BUILDFLAG(IS_ANDROID)

class PrefRegistrySimple;
class PrefService;

namespace misc_metrics {

extern const char kViewsMonthlyHistogramName[];
extern const char kIsEnabledHistogramName[];

// TODO(djandries): consider refactoring this into a more generic
// metrics service if we receive additional metric requests for features
// that don't have a mojo service that we can piggyback onto.
class PrivacyHubMetrics : public mojom::PrivacyHubMetrics {
 public:
  explicit PrivacyHubMetrics(PrefService* local_state);
  ~PrivacyHubMetrics() override;

  PrivacyHubMetrics(const PrivacyHubMetrics&) = delete;
  PrivacyHubMetrics& operator=(const PrivacyHubMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

#if BUILDFLAG(IS_ANDROID)
  mojo::PendingRemote<mojom::PrivacyHubMetrics> MakeRemote();
#endif  // BUILDFLAG(IS_ANDROID)

  void RecordView() override;
  void RecordEnabledStatus(bool is_enabled) override;

 private:
  void RecordViewCount();
  void SetUpTimer();

  MonthlyStorage view_storage_;
  base::WallClockTimer report_timer_;

#if BUILDFLAG(IS_ANDROID)
  mojo::ReceiverSet<mojom::PrivacyHubMetrics> receivers_;
#endif  // BUILDFLAG(IS_ANDROID)
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_PRIVACY_HUB_METRICS_H_
