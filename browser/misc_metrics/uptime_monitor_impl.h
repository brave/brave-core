/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_UPTIME_MONITOR_IMPL_H_
#define BRAVE_BROWSER_MISC_METRICS_UPTIME_MONITOR_IMPL_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/misc_metrics/uptime_monitor.h"
#include "brave/components/time_period_storage/weekly_storage.h"

class PrefService;
class PrefRegistrySimple;

namespace misc_metrics {

#if !BUILDFLAG(IS_ANDROID)
class UsageClock;
#endif

inline constexpr char kBrowserOpenTimeHistogramName[] =
    "Brave.Uptime.BrowserOpenTime.2";

class UptimeMonitorImpl : public UptimeMonitor {
 public:
  explicit UptimeMonitorImpl(PrefService* local_state);
  UptimeMonitorImpl(const UptimeMonitorImpl&) = delete;
  UptimeMonitorImpl& operator=(const UptimeMonitorImpl&) = delete;
  ~UptimeMonitorImpl() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);
  static void RegisterPrefsForMigration(PrefRegistrySimple* registry);
  static void MigrateObsoletePrefs(PrefService* local_state);

  void Init();

#if BUILDFLAG(IS_ANDROID)
  void ReportUsageDuration(base::TimeDelta duration);
#endif

  // UptimeMonitor:
  base::TimeDelta GetUsedTimeInWeek() const override;
  base::WeakPtr<UptimeMonitor> GetWeakPtr() override;
  bool IsInUse() const override;

 private:
  void RecordP3A();
#if !BUILDFLAG(IS_ANDROID)
  // Used on Desktop only.
  void RecordUsage();
#endif

  void ResetReportFrame();

  raw_ptr<PrefService> local_state_;

#if !BUILDFLAG(IS_ANDROID)
  std::unique_ptr<UsageClock> usage_clock_;

  base::TimeDelta current_total_usage_;
  base::RepeatingTimer timer_;
#endif

  base::Time report_frame_start_time_;
  base::TimeDelta report_frame_time_sum_;

  // Weekly storage for uptime data
  WeeklyStorage weekly_storage_;

  // WeakPtr factory for this class
  base::WeakPtrFactory<UptimeMonitor> weak_ptr_factory_{this};
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_UPTIME_MONITOR_IMPL_H_
