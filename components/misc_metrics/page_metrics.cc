/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/page_metrics.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/core/https_only_mode_metrics.h"

namespace misc_metrics {

namespace {

constexpr const int kPagesLoadedBuckets[] = {0, 10, 50, 100, 500, 1000};
constexpr const int kDomainsLoadedBuckets[] = {0, 4, 10, 30, 50, 100};
constexpr const int kFailedHTTPSUpgradeBuckets[] = {0, 25, 50, 100, 300, 700};

constexpr base::TimeDelta kReportInterval = base::Minutes(30);
constexpr base::TimeDelta kInitReportDelay = base::Seconds(30);

}  // namespace

using HttpsEvent = security_interstitials::https_only_mode::Event;

PageMetrics::PageMetrics(PrefService* local_state,
                         HostContentSettingsMap* host_content_settings_map,
                         history::HistoryService* history_service)
    : local_state_(local_state),
      host_content_settings_map_(host_content_settings_map),
      history_service_(history_service) {
  DCHECK(local_state);
  DCHECK(history_service);

  init_timer_.Start(FROM_HERE, kInitReportDelay, this,
                    &PageMetrics::ReportAllMetrics);

  https_navigation_event_observer_ =
      std::make_unique<base::StatisticsRecorder::ScopedHistogramSampleObserver>(
          security_interstitials::https_only_mode::kEventHistogram,
          base::BindRepeating(&PageMetrics::OnHttpsNavigationEvent,
                              weak_ptr_factory_.GetWeakPtr()));

  if (!local_state_->HasPrefPath(
          kMiscMetricsFailedHTTPSUpgradeMetricAddedTime)) {
    local_state_->SetTime(kMiscMetricsFailedHTTPSUpgradeMetricAddedTime,
                          base::Time::Now().LocalMidnight());
  }
}

PageMetrics::~PageMetrics() = default;

void PageMetrics::OnHttpsNavigationEvent(const char* histogram_name,
                                         uint64_t name_hash,
                                         base::HistogramBase::Sample sample) {
  HttpsEvent event = static_cast<HttpsEvent>(sample);
  if (event != HttpsEvent::kUpgradeFailed &&
      event != HttpsEvent::kUpgradeNetError) {
    return;
  }
  InitStorage();
  if (event == HttpsEvent::kUpgradeFailed) {
    failed_https_upgrades_storage_->AddDelta(1);
  } else {
    // If the upgrade failed due to a network error,
    // don't consider it in our metrics. kUpgradeFailed will be reported by
    // at the same time, so we need to subtract to compensate.
    failed_https_upgrades_storage_->SubDelta(1);
  }
}

void PageMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsPagesLoadedCount);
  registry->RegisterListPref(kMiscMetricsPagesReloadedCount);
  registry->RegisterListPref(kMiscMetricsHTTPAllowedLoadCount);
  registry->RegisterListPref(kMiscMetricsFailedHTTPSUpgradeCount);
  registry->RegisterTimePref(kMiscMetricsFailedHTTPSUpgradeMetricAddedTime, {});
}

void PageMetrics::IncrementPagesLoadedCount(bool is_reload) {
  VLOG(2) << "PageMetricsService: increment page load count, is_reload "
          << is_reload;
  InitStorage();
  if (is_reload) {
    pages_reloaded_storage_->AddDelta(1);
  } else {
    pages_loaded_storage_->AddDelta(1);
  }
}

void PageMetrics::RecordAllowedHTTPRequest() {
  InitStorage();
  http_allowed_pages_loaded_storage_->AddDelta(1);
}

void PageMetrics::InitStorage() {
  if (pages_reloaded_storage_ == nullptr) {
    pages_reloaded_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsPagesReloadedCount);
  }
  if (pages_loaded_storage_ == nullptr) {
    pages_loaded_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsPagesLoadedCount);
  }
  if (http_allowed_pages_loaded_storage_ == nullptr) {
    http_allowed_pages_loaded_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsHTTPAllowedLoadCount);
  }
  if (failed_https_upgrades_storage_ == nullptr) {
    failed_https_upgrades_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsFailedHTTPSUpgradeCount);
  }
}

void PageMetrics::ReportAllMetrics() {
  InitStorage();
  ReportDomainsLoaded();
  ReportPagesLoaded();
  ReportFailedHTTPSUpgrades();
  periodic_report_timer_.Start(
      FROM_HERE, base::Time::Now() + kReportInterval,
      base::BindOnce(&PageMetrics::ReportAllMetrics, base::Unretained(this)));
}

void PageMetrics::ReportDomainsLoaded() {
  // Derived from current profile history.
  // Mutiple profiles will result in metric overwrites which is okay.
  history_service_->GetDomainDiversity(
      base::Time::Now(), /*number_of_days_to_report*/ 1,
      history::DomainMetricType::kEnableLast7DayMetric,
      base::BindOnce(&PageMetrics::OnDomainDiversityResult,
                     base::Unretained(this)),
      &history_service_task_tracker_);
}

void PageMetrics::ReportPagesLoaded() {
  // Stores a global count in local state to
  // capture page loads across all profiles.
  uint64_t pages_loaded_count = pages_loaded_storage_->GetWeeklySum();
  uint64_t pages_reloaded_count = pages_reloaded_storage_->GetWeeklySum();
  p3a_utils::RecordToHistogramBucket(kPagesLoadedHistogramName,
                                     kPagesLoadedBuckets, pages_loaded_count);
  p3a_utils::RecordToHistogramBucket(kPagesReloadedHistogramName,
                                     kPagesLoadedBuckets, pages_reloaded_count);
  VLOG(2) << "PageMetricsService: pages loaded report, loaded count = "
          << pages_loaded_count << " reloaded count = " << pages_reloaded_count;
}

void PageMetrics::ReportFailedHTTPSUpgrades() {
  brave_shields::ControlType https_upgrade_settings =
      brave_shields::GetHttpsUpgradeControlType(host_content_settings_map_,
                                                GURL());
  if (https_upgrade_settings == brave_shields::ControlType::ALLOW) {
    if (local_state_->HasPrefPath(
            kMiscMetricsFailedHTTPSUpgradeMetricAddedTime)) {
      // If the metric was recorded in the past, but HTTPS-First or HTTPS-Only
      // mode is not enabled, clear all prefs to save on storage.
      http_allowed_pages_loaded_storage_ = nullptr;
      failed_https_upgrades_storage_ = nullptr;
      local_state_->ClearPref(kMiscMetricsFailedHTTPSUpgradeMetricAddedTime);
      local_state_->ClearPref(kMiscMetricsFailedHTTPSUpgradeCount);
      local_state_->ClearPref(kMiscMetricsHTTPAllowedLoadCount);
      InitStorage();
    }
    // Don't report metric if HTTPS-First or HTTPS-Only mode is not enabled.
    return;
  }

  if (!local_state_->HasPrefPath(
          kMiscMetricsFailedHTTPSUpgradeMetricAddedTime)) {
    local_state_->SetTime(kMiscMetricsFailedHTTPSUpgradeMetricAddedTime,
                          base::Time::Now().LocalMidnight());
  }

  base::Time metric_added_time =
      local_state_->GetTime(kMiscMetricsFailedHTTPSUpgradeMetricAddedTime);

  uint64_t pages_loaded;
  if ((base::Time::Now() - metric_added_time) < base::Days(7)) {
    base::Time now = base::Time::Now().LocalMidnight();
    pages_loaded =
        pages_loaded_storage_->GetPeriodSumInTimeRange(metric_added_time, now) +
        pages_reloaded_storage_->GetPeriodSumInTimeRange(metric_added_time,
                                                         now);
  } else {
    pages_loaded = pages_loaded_storage_->GetWeeklySum() +
                   pages_reloaded_storage_->GetWeeklySum();
  }

  uint64_t allowed_http_loads =
      http_allowed_pages_loaded_storage_->GetWeeklySum();
  uint64_t failed_https_upgrades =
      failed_https_upgrades_storage_->GetWeeklySum();

  // We want to exclude allowlisted HTTP loads from the denominator
  // (i.e. sites that were previously loaded via HTTP). We do not want to
  // consider them in the full ratio, so we can prevent skewing. is there a
  // better way to do this? all types are uint64_t
  uint64_t denominator = allowed_http_loads <= pages_loaded
                             ? pages_loaded - allowed_http_loads
                             : 0;

  double percentage = 0;
  if (denominator > 0) {
    percentage = static_cast<double>(failed_https_upgrades) / denominator * 100;
  }
  VLOG(2) << "PageMetrics: failed https upgrade report, failed upgrades = "
          << failed_https_upgrades << ", denominator = " << denominator
          << ", percentage = " << percentage;
  p3a_utils::RecordToHistogramBucket(kFailedHTTPSUpgradesHistogramName,
                                     kFailedHTTPSUpgradeBuckets,
                                     percentage * 100);
}

void PageMetrics::OnDomainDiversityResult(
    std::pair<history::DomainDiversityResults, history::DomainDiversityResults>
        metrics) {
  if (metrics.first.empty() || metrics.second.empty()) {
    return;
  }
  // The second entry in the pair counts both local, and foreign (synced)
  // visits.
  const history::DomainMetricSet& metric_set = metrics.first.front();
  if (!metric_set.seven_day_metric.has_value()) {
    return;
  }
  int count = metric_set.seven_day_metric->count;
  p3a_utils::RecordToHistogramBucket(kDomainsLoadedHistogramName,
                                     kDomainsLoadedBuckets, count);
  VLOG(2) << "PageMetrics: domains loaded report, count = " << count;
}

}  // namespace misc_metrics
