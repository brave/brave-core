// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/misc_metrics/new_tab_metrics.h"

#include <utility>

#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

namespace {

enum class NTPSearchEngine {
  kBrave = 0,
  kGoogle,
  kDDG,
  kQwant,
  kBing,
  kStartpage,
  kEcosia,
  kOther,
  kMaxValue = kOther
};
constexpr base::TimeDelta kUpdateInterval = base::Days(1);
constexpr int kUsageBuckets[] = {10, 30, 40};

}  // namespace

using TemplateURLPrepopulateData::BravePrepopulatedEngineID;

NewTabMetrics::NewTabMetrics(PrefService* local_state)
    : usage_storage_(local_state, kMiscMetricsNTPWidgetUsageStorage) {
  ReportCounts();
}
NewTabMetrics::~NewTabMetrics() = default;

void NewTabMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsNTPWidgetUsageStorage);
}

void NewTabMetrics::Bind(
    mojo::PendingReceiver<brave_new_tab_page::mojom::NewTabMetrics> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void NewTabMetrics::ReportNTPSearchDefaultEngine(
    std::optional<int64_t> prepopulate_id) {
  if (!prepopulate_id) {
    UMA_HISTOGRAM_EXACT_LINEAR(kNTPSearchEngineHistogramName, INT_MAX - 1, 8);
    return;
  }

  NTPSearchEngine search_engine = NTPSearchEngine::kOther;

  switch (*prepopulate_id) {
    case BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_DUCKDUCKGO:
    case BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_DE:
    case BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_DUCKDUCKGO_AU_NZ_IE:
      search_engine = NTPSearchEngine::kDDG;
      break;
    case BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_BRAVE:
      search_engine = NTPSearchEngine::kBrave;
      break;
    case BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_BING:
      search_engine = NTPSearchEngine::kBing;
      break;
    case BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_QWANT:
      search_engine = NTPSearchEngine::kQwant;
      break;
    case BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_STARTPAGE:
      search_engine = NTPSearchEngine::kStartpage;
      break;
    case BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_ECOSIA:
      search_engine = NTPSearchEngine::kEcosia;
      break;
    case BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_GOOGLE:
      search_engine = NTPSearchEngine::kGoogle;
      break;
  }

  UMA_HISTOGRAM_ENUMERATION(kNTPSearchEngineHistogramName, search_engine);
}

void NewTabMetrics::ReportNTPSearchUsage(int64_t prepopulate_id) {
  if (prepopulate_id ==
      BravePrepopulatedEngineID::PREPOPULATED_ENGINE_ID_GOOGLE) {
    UMA_HISTOGRAM_BOOLEAN(kNTPGoogleWidgetUsageHistogramName, true);
  }
  usage_storage_.AddDelta(1);
  ReportCounts();
}

void NewTabMetrics::ReportCounts() {
  int usage_count = usage_storage_.GetWeeklySum();
  if (usage_count > 0) {
    p3a_utils::RecordToHistogramBucket(kNTPSearchUsageHistogramName,
                                       kUsageBuckets, usage_count);
  }
  update_timer_.Start(FROM_HERE, base::Time::Now() + kUpdateInterval, this,
                      &NewTabMetrics::ReportCounts);
}

}  // namespace misc_metrics
