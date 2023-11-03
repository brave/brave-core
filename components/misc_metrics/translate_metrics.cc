/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/translate_metrics.h"

#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kReportInterval = base::Days(1);
constexpr char kPageCountHistogramName[] = "Brave.Translate.PageCount";
const int kPageCountBuckets[] = {1, 5, 10};

}  // namespace

TranslateMetrics::TranslateMetrics(PrefService* local_state)
    : translation_count_(local_state, kMiscMetricsTranslationPageCount) {
  UpdateMetrics();
}

TranslateMetrics::~TranslateMetrics() = default;

void TranslateMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsTranslationPageCount);
}

void TranslateMetrics::RecordPageTranslation() {
  translation_count_.AddDelta(1u);
  ReportPageTranslationCount();
}

void TranslateMetrics::UpdateMetrics() {
  ReportPageTranslationCount();
  daily_timer_.Start(
      FROM_HERE, base::Time::Now() + kReportInterval,
      base::BindOnce(&TranslateMetrics::UpdateMetrics, base::Unretained(this)));
}

void TranslateMetrics::ReportPageTranslationCount() {
  uint64_t total = translation_count_.GetWeeklySum();
  if (total == 0) {
    return;
  }
  p3a_utils::RecordToHistogramBucket(kPageCountHistogramName, kPageCountBuckets,
                                     total);
}

}  // namespace misc_metrics
