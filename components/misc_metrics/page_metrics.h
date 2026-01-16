/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_PAGE_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_PAGE_METRICS_H_

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/statistics_recorder.h"
#include "base/scoped_observation.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/timer/timer.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/misc_metrics/default_browser_monitor.h"
#include "components/browsing_data/core/counters/browsing_data_counter.h"
#include "components/history/core/browser/history_types.h"
#include "components/prefs/pref_change_registrar.h"

class HostContentSettingsMap;
class PrefRegistrySimple;
class PrefService;
class WeeklyStorage;

namespace browsing_data {
class BookmarkCounter;
}  // namespace browsing_data

namespace bookmarks {
class BookmarkModel;
}  // namespace bookmarks

namespace history {
class HistoryService;
}  // namespace history

namespace misc_metrics {

inline constexpr char kPagesLoadedNonRewardsHistogramName[] =
    "Brave.Core.PagesLoaded.NonRewards";
inline constexpr char kPagesLoadedRewardsHistogramName[] =
    "Brave.Core.PagesLoaded.Rewards";
inline constexpr char kPagesLoadedRewardsWalletHistogramName[] =
    "Brave.Core.PagesLoaded.RewardsWallet";
inline constexpr char kPagesReloadedHistogramName[] =
    "Brave.Core.PagesReloaded";
inline constexpr char kDomainsLoadedDefaultHistogramName[] =
    "Brave.Core.DomainsLoaded.Default";
inline constexpr char kDomainsLoadedNonDefaultHistogramName[] =
    "Brave.Core.DomainsLoaded.NonDefault";
inline constexpr char kFailedHTTPSUpgradesHistogramName[] =
    "Brave.Core.FailedHTTPSUpgrades.2";
inline constexpr char kBookmarkCountHistogramName[] =
    "Brave.Core.BookmarkCount";
inline constexpr char kFirstPageLoadTimeHistogramName[] =
    "Brave.Core.FirstPageLoadTime";
inline constexpr char kSearchBraveDailyHistogramName[] =
    "Brave.Search.BraveDaily.2";

// Manages browser page loading metrics, including page load counts,
// failed HTTPS upgrades, and bookmarks.
class PageMetrics : public DefaultBrowserMonitor::Observer {
 public:
  using FirstRunTimeCallback = base::RepeatingCallback<base::Time(void)>;

  PageMetrics(PrefService* local_state,
              PrefService* profile_prefs,
              HostContentSettingsMap* host_content_settings_map,
              history::HistoryService* history_service,
              bookmarks::BookmarkModel* bookmark_model,
              DefaultBrowserMonitor* default_browser_monitor,
              FirstRunTimeCallback first_run_time_callback);
  ~PageMetrics() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void IncrementPagesLoadedCount(bool is_reload);

  void ReportBraveQuery();

  // DefaultBrowserMonitor::Observer:
  void OnDefaultBrowserStatusChanged() override;

 private:
  void InitStorage();

  void ReportAllMetrics();
  void ReportDomainsLoaded();
  void ReportPagesLoaded();
  void ReportFailedHTTPSUpgrades();
  void ReportBookmarkCount();

  void ReportFirstPageLoadTime();

  void OnHttpsNavigationEvent(std::string_view histogram_name,
                              uint64_t name_hash,
                              base::HistogramBase::Sample32 sample);
  void OnInterstitialDecisionEvent(std::string_view histogram_name,
                                   uint64_t name_hash,
                                   base::HistogramBase::Sample32 sample);

  void OnDomainDiversityResult(
      std::pair<history::DomainDiversityResults,
                history::DomainDiversityResults> result);

  void OnBookmarkCountResult(
      std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result);

  void ReportDomainsLoadedWithStatus();

  std::unique_ptr<WeeklyStorage> pages_loaded_storage_;
  std::unique_ptr<WeeklyStorage> pages_reloaded_storage_;
  std::unique_ptr<WeeklyStorage> interstitial_allow_decisions_storage_;
  std::unique_ptr<WeeklyStorage> failed_https_upgrades_storage_;

  base::CancelableTaskTracker history_service_task_tracker_;

  base::WallClockTimer periodic_report_timer_;
  base::OneShotTimer init_timer_;

  std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>
      https_navigation_event_observer_;
  std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>
      interstitial_decision_observer_;

  std::unique_ptr<browsing_data::BookmarkCounter> bookmark_counter_;

  raw_ptr<PrefService> local_state_ = nullptr;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  raw_ptr<history::HistoryService> history_service_ = nullptr;

  FirstRunTimeCallback first_run_time_callback_;
  base::Time first_run_time_;

  std::optional<int> current_domain_count_;
  bool has_pending_brave_query_ = false;

  raw_ptr<DefaultBrowserMonitor> default_browser_monitor_;
  base::ScopedObservation<DefaultBrowserMonitor,
                          DefaultBrowserMonitor::Observer>
      default_browser_observation_{this};

  PrefChangeRegistrar pref_change_registrar_;

  base::WeakPtrFactory<PageMetrics> weak_ptr_factory_{this};
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_PAGE_METRICS_H_
