/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_TRANSLATE_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_TRANSLATE_METRICS_H_

#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/time_period_storage/weekly_storage.h"

class PrefRegistrySimple;
class PrefService;

namespace misc_metrics {

class TranslateMetrics {
 public:
  explicit TranslateMetrics(PrefService* local_state);
  ~TranslateMetrics();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  TranslateMetrics(const TranslateMetrics&) = delete;
  TranslateMetrics& operator=(const TranslateMetrics&) = delete;

  void RecordPageTranslation();

 private:
  void UpdateMetrics();
  void ReportPageTranslationCount();

  base::WallClockTimer daily_timer_;
  WeeklyStorage translation_count_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_TRANSLATE_METRICS_H_
