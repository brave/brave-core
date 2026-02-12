/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_

#include <memory>
#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/template_url_service_observer.h"

class DailyStorage;
class PrefRegistrySimple;
class PrefService;
class TemplateURLService;

namespace misc_metrics {

inline constexpr char kSearchDailyQueriesBraveDefaultHistogramName[] =
    "Brave.Search.DailyQueries.BraveDefault";
inline constexpr char kSearchDailyQueriesGoogleDefaultHistogramName[] =
    "Brave.Search.DailyQueries.GoogleDefault";
inline constexpr char kSearchDailyQueriesOtherDefaultHistogramName[] =
    "Brave.Search.DailyQueries.OtherDefault";

class BraveSearchMetrics : public TemplateURLServiceObserver {
 public:
  BraveSearchMetrics(PrefService* local_state,
                     TemplateURLService* template_url_service);
  ~BraveSearchMetrics() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordBraveQuery();

  void ReportDailyQueries();

 private:
  void UpdateSearchEngineType();

  // TemplateURLServiceObserver:
  void OnTemplateURLServiceChanged() override;

  raw_ptr<PrefService> local_state_ = nullptr;
  raw_ptr<TemplateURLService> template_url_service_ = nullptr;
  base::ScopedObservation<TemplateURLService, TemplateURLServiceObserver>
      template_url_service_observation_{this};

  std::unique_ptr<DailyStorage> brave_search_daily_queries_storage_;
  std::optional<SearchEngineType> default_search_engine_type_;

  base::WeakPtrFactory<BraveSearchMetrics> weak_ptr_factory_{this};
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_BRAVE_SEARCH_METRICS_H_
