/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_PAGE_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_PAGE_METRICS_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/metrics/histogram_base.h"
#include "base/metrics/statistics_recorder.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/timer/timer.h"
#include "base/timer/wall_clock_timer.h"
#include "components/history/core/browser/history_types.h"

class HostContentSettingsMap;
class PrefRegistrySimple;
class PrefService;
class WeeklyStorage;

namespace history {
class HistoryService;
}  // namespace history

namespace misc_metrics {

inline constexpr char kPagesLoadedHistogramName[] = "Brave.Core.PagesLoaded.2";
inline constexpr char kPagesReloadedHistogramName[] =
    "Brave.Core.PagesReloaded";
inline constexpr char kDomainsLoadedHistogramName[] =
    "Brave.Core.DomainsLoaded";
inline constexpr char kFailedHTTPSUpgradesHistogramName[] =
    "Brave.Core.FailedHTTPSUpgrades";

class PageMetrics {
 public:
  PageMetrics(PrefService* local_state,
              HostContentSettingsMap* host_content_settings_map,
              history::HistoryService* history_service);
  ~PageMetrics();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void IncrementPagesLoadedCount(bool is_reload);

  void RecordAllowedHTTPRequest();

 private:
  void InitStorage();

  void ReportAllMetrics();
  void ReportDomainsLoaded();
  void ReportPagesLoaded();
  void ReportFailedHTTPSUpgrades();

  void OnHttpsNavigationEvent(const char* histogram_name,
                              uint64_t name_hash,
                              base::HistogramBase::Sample sample);

  void OnDomainDiversityResult(
      std::pair<history::DomainDiversityResults,
                history::DomainDiversityResults> result);

  std::unique_ptr<WeeklyStorage> pages_loaded_storage_;
  std::unique_ptr<WeeklyStorage> pages_reloaded_storage_;
  std::unique_ptr<WeeklyStorage> http_allowed_pages_loaded_storage_;
  std::unique_ptr<WeeklyStorage> failed_https_upgrades_storage_;

  base::CancelableTaskTracker history_service_task_tracker_;

  base::WallClockTimer periodic_report_timer_;
  base::OneShotTimer init_timer_;

  std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>
      https_navigation_event_observer_;

  raw_ptr<PrefService> local_state_ = nullptr;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  raw_ptr<history::HistoryService> history_service_ = nullptr;

  base::WeakPtrFactory<PageMetrics> weak_ptr_factory_{this};
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_PAGE_METRICS_H_
