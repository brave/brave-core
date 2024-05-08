/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_UPTIME_MONITOR_H_
#define BRAVE_BROWSER_MISC_METRICS_UPTIME_MONITOR_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"

class PrefService;
class PrefRegistrySimple;

#if !BUILDFLAG(IS_ANDROID)
namespace resource_coordinator {
class UsageClock;
}  // namespace resource_coordinator
#endif

namespace misc_metrics {

inline constexpr char kBrowserOpenTimeHistogramName[] =
    "Brave.Uptime.BrowserOpenTime.2";

class UptimeMonitor {
 public:
  explicit UptimeMonitor(PrefService* local_state);
  UptimeMonitor(const UptimeMonitor&) = delete;
  UptimeMonitor& operator=(const UptimeMonitor&) = delete;
  ~UptimeMonitor();

  static void RegisterPrefs(PrefRegistrySimple* registry);
  static void RegisterPrefsForMigration(PrefRegistrySimple* registry);
  static void MigrateObsoletePrefs(PrefService* local_state);

  void Init();

#if BUILDFLAG(IS_ANDROID)
  void ReportUsageDuration(base::TimeDelta duration);
#endif

 private:
  void RecordP3A();
#if !BUILDFLAG(IS_ANDROID)
  // Used on Desktop only.
  void RecordUsage();
#endif

  void ResetReportFrame();

  raw_ptr<PrefService> local_state_;

#if !BUILDFLAG(IS_ANDROID)
  std::unique_ptr<resource_coordinator::UsageClock> usage_clock_;

  base::TimeDelta current_total_usage_;
  base::RepeatingTimer timer_;
#endif

  base::Time report_frame_start_time_;
  base::TimeDelta report_frame_time_sum_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_UPTIME_MONITOR_H_
