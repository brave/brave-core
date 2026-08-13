/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_p3a.h"

#include <climits>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/serp_metrics/serp_metrics_all_profiles_aggregator.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/serp_metrics/pref_names.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace serp_metrics {

namespace {

constexpr char kBraveSerpDictKey[] = "brave";
constexpr char kGoogleSerpDictKey[] = "google";
constexpr char kOtherSerpDictKey[] = "other";
constexpr char kStaleSerpDictKey[] = "stale";

constexpr auto kSerpHistogramToDictKey =
    base::MakeFixedFlatMap<std::string_view, std::string_view>({
        {kBraveSerpHistogramName, kBraveSerpDictKey},
        {kGoogleSerpHistogramName, kGoogleSerpDictKey},
        {kOtherSerpHistogramName, kOtherSerpDictKey},
        {kStaleSerpHistogramName, kStaleSerpDictKey},
    });

// P3A bucket boundaries (inclusive upper bounds).
constexpr int kBraveSerpBuckets[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 17};
constexpr int kGoogleSerpBuckets[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 13, 21, 34};
constexpr int kOtherSerpBuckets[] = {0, 1, 2, 3, 4};
constexpr int kStaleSerpBuckets[] = {0, 1, 2, 3, 4, 5, 10, 15};

template <std::size_t N>
void ReportMetric(const char* histogram_name,
                  const int (&buckets)[N],
                  size_t count,
                  bool suppress_when_zero) {
  if (suppress_when_zero && count == 0) {
    base::UmaHistogramExactLinear(histogram_name, INT_MAX - 1,
                                  std::size(buckets) + 1);
    return;
  }
  p3a_utils::RecordToHistogramBucket(histogram_name, buckets,
                                     static_cast<int>(count));
}

}  // namespace

SerpMetricsP3A::SerpMetricsP3A(PrefService& local_state)
    : local_state_(local_state) {}

SerpMetricsP3A::~SerpMetricsP3A() = default;

void SerpMetricsP3A::Init() {
  if (!base::FeatureList::IsEnabled(kSerpMetricsFeature) ||
      !kSerpMetricsP3A.Get()) {
    return;
  }

  p3a::P3AService* p3a_service = g_brave_browser_process->p3a_service();
  if (!p3a_service) {
    CHECK_IS_TEST();
    return;
  }

  rotation_subscription_ = p3a_service->RegisterRotationCallback(
      base::BindRepeating(&SerpMetricsP3A::OnRotation, base::Unretained(this)));
  metric_cycled_subscription_ =
      p3a_service->RegisterMetricCycledCallback(base::BindRepeating(
          &SerpMetricsP3A::OnMetricCycled, base::Unretained(this)));
}

// static
void SerpMetricsP3A::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kP3ALastReportedAtDict);
}

void SerpMetricsP3A::OnRotation(p3a::MetricLogType log_type) {
  if (log_type != p3a::MetricLogType::kExpress) {
    return;
  }
  ReportMetrics();
}

void SerpMetricsP3A::OnMetricCycled(const std::string& histogram_name) {
  const std::string_view* dict_key =
      base::FindOrNull(kSerpHistogramToDictKey, histogram_name);
  if (!dict_key) {
    return;
  }
  ScopedDictPrefUpdate update(local_state_.get(),
                              prefs::kP3ALastReportedAtDict);
  update->Set(*dict_key, base::TimeToValue(base::Time::Now()));
}

void SerpMetricsP3A::ReportMetrics() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  CHECK(profile_manager);

  SerpMetricsAllProfilesAggregator aggregator(
      &local_state_.get(), profile_manager->GetProfileAttributesStorage());

  const size_t brave_count = aggregator.GetSearchCountForYesterday(
      SerpMetricType::kBrave, GetLastReportedTime(kBraveSerpDictKey));
  const size_t google_count = aggregator.GetSearchCountForYesterday(
      SerpMetricType::kGoogle, GetLastReportedTime(kGoogleSerpDictKey));
  const size_t other_count = aggregator.GetSearchCountForYesterday(
      SerpMetricType::kOther, GetLastReportedTime(kOtherSerpDictKey));
  const size_t stale_count = aggregator.GetSearchCountForStalePeriod(
      GetLastReportedTime(kStaleSerpDictKey));

  ReportMetric(kBraveSerpHistogramName, kBraveSerpBuckets, brave_count,
               /*suppress_when_zero=*/false);
  ReportMetric(kGoogleSerpHistogramName, kGoogleSerpBuckets, google_count,
               /*suppress_when_zero=*/true);
  ReportMetric(kOtherSerpHistogramName, kOtherSerpBuckets, other_count,
               /*suppress_when_zero=*/true);
  ReportMetric(kStaleSerpHistogramName, kStaleSerpBuckets, stale_count,
               /*suppress_when_zero=*/true);
}

base::Time SerpMetricsP3A::GetLastReportedTime(
    std::string_view dict_key) const {
  const base::Value* value =
      local_state_->GetDict(prefs::kP3ALastReportedAtDict).Find(dict_key);
  return value ? base::ValueToTime(*value).value_or(base::Time())
               : base::Time();
}

}  // namespace serp_metrics
