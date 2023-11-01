/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/page_metrics_service.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

const char kPagesLoadedHistogramName[] = "Brave.Core.PagesLoaded";
const char kDomainsLoadedHistogramName[] = "Brave.Core.DomainsLoaded";

namespace {

constexpr const int kPagesLoadedBuckets[] = {0, 10, 50, 100, 500, 1000};
constexpr const int kDomainsLoadedBuckets[] = {0, 4, 10, 30, 50, 100};

constexpr base::TimeDelta kPagesLoadedReportInterval = base::Minutes(30);
constexpr base::TimeDelta kDomainsLoadedReportInterval = base::Minutes(30);
constexpr base::TimeDelta kPagesLoadedInitReportDelay = base::Seconds(30);
constexpr base::TimeDelta kDomainsLoadedInitReportDelay = base::Seconds(30);

}  // namespace

PageMetricsService::PageMetricsService(PrefService* local_state,
                                       history::HistoryService* history_service)
    : local_state_(local_state), history_service_(history_service) {
  // history_service_ can be nulltpr in unit tests.
  DCHECK(local_state);

  pages_loaded_report_timer_.Start(FROM_HERE, kPagesLoadedReportInterval, this,
                                   &PageMetricsService::ReportPagesLoaded);
  domains_loaded_report_timer_.Start(FROM_HERE, kDomainsLoadedReportInterval,
                                     this,
                                     &PageMetricsService::ReportDomainsLoaded);

  pages_loaded_report_init_timer_.Start(FROM_HERE, kPagesLoadedInitReportDelay,
                                        this,
                                        &PageMetricsService::ReportPagesLoaded);
  domains_loaded_report_init_timer_.Start(
      FROM_HERE, kDomainsLoadedInitReportDelay, this,
      &PageMetricsService::ReportDomainsLoaded);
}

PageMetricsService::~PageMetricsService() = default;

void PageMetricsService::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsPagesLoadedCount);
}

void PageMetricsService::IncrementPagesLoadedCount() {
  VLOG(2) << "PageMetricsService: increment page load count";
  if (pages_loaded_storage_ == nullptr) {
    pages_loaded_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsPagesLoadedCount);
  }
  pages_loaded_storage_->AddDelta(1);
}

void PageMetricsService::ReportDomainsLoaded() {
  // history_service_ can be nulltpr in unit tests.
  if (!history_service_) {
    return;
  }
  // Derived from current profile history.
  // Mutiple profiles will result in metric overwrites which is okay.
  history_service_->GetDomainDiversity(
      base::Time::Now(), /*number_of_days_to_report*/ 1,
      history::DomainMetricType::kEnableLast7DayMetric,
      base::BindOnce(&PageMetricsService::OnDomainDiversityResult,
                     base::Unretained(this)),
      &history_service_task_tracker_);
}

void PageMetricsService::ReportPagesLoaded() {
  // Stores a global count in local state to
  // capture page loads across all profiles.
  if (pages_loaded_storage_ == nullptr) {
    pages_loaded_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsPagesLoadedCount);
  }
  uint64_t count = pages_loaded_storage_->GetPeriodSum();
  p3a_utils::RecordToHistogramBucket(kPagesLoadedHistogramName,
                                     kPagesLoadedBuckets, count);
  VLOG(2) << "PageMetricsService: pages loaded report, count = " << count;
}

void PageMetricsService::OnDomainDiversityResult(
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
  VLOG(2) << "PageMetricsService: domains loaded report, count = " << count;
}

}  // namespace misc_metrics
