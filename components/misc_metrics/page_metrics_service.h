/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_PAGE_METRICS_SERVICE_H_
#define BRAVE_COMPONENTS_MISC_METRICS_PAGE_METRICS_SERVICE_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/task/cancelable_task_tracker.h"
#include "base/timer/timer.h"
#include "components/history/core/browser/history_types.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class PrefService;
class WeeklyStorage;

namespace history {
class HistoryService;
}  // namespace history

namespace misc_metrics {

inline constexpr char kPagesLoadedHistogramName[] = "Brave.Core.PagesLoaded";
inline constexpr char kDomainsLoadedHistogramName[] =
    "Brave.Core.DomainsLoaded";

class PageMetricsService : public KeyedService {
 public:
  PageMetricsService(PrefService* local_state,
                     history::HistoryService* history_service);
  ~PageMetricsService() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void IncrementPagesLoadedCount();

 private:
  void ReportDomainsLoaded();
  void ReportPagesLoaded();

  void OnDomainDiversityResult(
      std::pair<history::DomainDiversityResults,
                history::DomainDiversityResults> result);

  std::unique_ptr<WeeklyStorage> pages_loaded_storage_;

  base::CancelableTaskTracker history_service_task_tracker_;

  base::RepeatingTimer domains_loaded_report_timer_;
  base::RepeatingTimer pages_loaded_report_timer_;
  base::OneShotTimer domains_loaded_report_init_timer_;
  base::OneShotTimer pages_loaded_report_init_timer_;

  raw_ptr<PrefService> local_state_ = nullptr;
  raw_ptr<history::HistoryService> history_service_ = nullptr;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_PAGE_METRICS_SERVICE_H_
