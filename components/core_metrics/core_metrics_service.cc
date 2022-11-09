/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/core_metrics/core_metrics_service.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "brave/components/core_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace core_metrics {

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

CoreMetricsService::CoreMetricsService(PrefService* local_state,
                                       history::HistoryService* history_service)
    : local_state_(local_state), history_service_(history_service) {
  DCHECK(local_state);
  DCHECK(history_service);

  pages_loaded_report_timer_.Start(FROM_HERE, kPagesLoadedReportInterval, this,
                                   &CoreMetricsService::ReportPagesLoaded);
  domains_loaded_report_timer_.Start(FROM_HERE, kDomainsLoadedReportInterval,
                                     this,
                                     &CoreMetricsService::ReportDomainsLoaded);

  pages_loaded_report_init_timer_.Start(FROM_HERE, kPagesLoadedInitReportDelay,
                                        this,
                                        &CoreMetricsService::ReportPagesLoaded);
  domains_loaded_report_init_timer_.Start(
      FROM_HERE, kDomainsLoadedInitReportDelay, this,
      &CoreMetricsService::ReportDomainsLoaded);
}

CoreMetricsService::~CoreMetricsService() = default;

void CoreMetricsService::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kCoreMetricsPagesLoadedCount);
}

void CoreMetricsService::IncrementPagesLoadedCount() {
  VLOG(2) << "CoreMetricsService: increment page load count";
  if (pages_loaded_storage_ == nullptr) {
    pages_loaded_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kCoreMetricsPagesLoadedCount);
  }
  pages_loaded_storage_->AddDelta(1);
}

void CoreMetricsService::ReportDomainsLoaded() {
  // Derived from current profile history.
  // Mutiple profiles will result in metric overwrites which is okay.
  history_service_->GetDomainDiversity(
      base::Time::Now(), /*number_of_days_to_report*/ 1,
      history::DomainMetricType::kEnableLast7DayMetric,
      base::BindOnce(&CoreMetricsService::OnDomainDiversityResult,
                     base::Unretained(this)),
      &history_service_task_tracker_);
}

void CoreMetricsService::ReportPagesLoaded() {
  // Stores a global count in local state to
  // capture page loads across all profiles.
  if (pages_loaded_storage_ == nullptr) {
    pages_loaded_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kCoreMetricsPagesLoadedCount);
  }
  uint64_t count = pages_loaded_storage_->GetPeriodSum();
  p3a_utils::RecordToHistogramBucket(kPagesLoadedHistogramName,
                                     kPagesLoadedBuckets, count);
  VLOG(2) << "CoreMetricsService: pages loaded report, count = " << count;
}

void CoreMetricsService::OnDomainDiversityResult(
    std::vector<history::DomainMetricSet> metrics) {
  if (metrics.size() == 0) {
    return;
  }
  const history::DomainMetricSet& metric_set = metrics[0];
  if (!metric_set.seven_day_metric.has_value()) {
    return;
  }
  int count = metric_set.seven_day_metric->count;
  p3a_utils::RecordToHistogramBucket(kDomainsLoadedHistogramName,
                                     kDomainsLoadedBuckets, count);
  VLOG(2) << "CoreMetricsService: domains loaded report, count = " << count;
}

}  // namespace core_metrics
