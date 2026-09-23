/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace speedreader {

namespace {

constexpr int kPageViewsBuckets[] = {5, 10, 20, 30};

constexpr base::TimeDelta kUpdateInterval = base::Days(1);

enum class EnabledSitesMetricValue {
  kNone = 0,      // No sites enabled
  kOne = 1,       // 1 site enabled
  kMultiple = 2,  // 2+ sites enabled
  kAll = 3,       // All sites enabled
  kMaxValue = kAll
};

}  // namespace

SpeedreaderMetrics::SpeedreaderMetrics(
    PrefService* local_state,
    HostContentSettingsMap* host_content_settings_map,
    bool is_allowed_for_all_readable_sites)
    : page_views_storage_(local_state, kSpeedreaderPageViewsStoragePref),
      host_content_settings_map_(host_content_settings_map),
      local_state_(local_state) {
  ReportPageViews();
  UpdateEnabledSitesMetric(is_allowed_for_all_readable_sites);
}

SpeedreaderMetrics::~SpeedreaderMetrics() = default;

// static
void SpeedreaderMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kSpeedreaderPageViewsStoragePref);
}

void SpeedreaderMetrics::RecordPageView() {
  page_views_storage_.AddDelta(1);
  ReportPageViews();
}

void SpeedreaderMetrics::UpdateEnabledSitesMetric(
    bool is_allowed_for_all_readable_sites) {
  EnabledSitesMetricValue value = EnabledSitesMetricValue::kNone;

  if (is_allowed_for_all_readable_sites) {
    value = EnabledSitesMetricValue::kAll;
  } else if (host_content_settings_map_) {
    auto settings = host_content_settings_map_->GetSettingsForOneType(
        ContentSettingsType::BRAVE_SPEEDREADER);
    size_t enabled_sites_count =
        std::ranges::count_if(settings, [](const auto& setting) {
          return setting.GetContentSetting() == CONTENT_SETTING_ALLOW;
        });
    if (enabled_sites_count == 1) {
      value = EnabledSitesMetricValue::kOne;
    } else if (enabled_sites_count > 1) {
      value = EnabledSitesMetricValue::kMultiple;
    }
  }

  UMA_HISTOGRAM_ENUMERATION(kSpeedreaderEnabledSitesHistogramName, value);
}

void SpeedreaderMetrics::ReportPageViews() {
  int page_views = page_views_storage_.GetMonthlySum();
  if (page_views > 0) {
    p3a_utils::RecordToHistogramBucket(kSpeedreaderPageViewsHistogramName,
                                       kPageViewsBuckets, page_views);
  }

  update_timer_.Start(FROM_HERE, base::Time::Now() + kUpdateInterval, this,
                      &SpeedreaderMetrics::ReportPageViews);
}

}  // namespace speedreader
