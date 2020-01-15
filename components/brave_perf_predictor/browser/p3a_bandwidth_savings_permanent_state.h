/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_P3A_BANDWIDTH_SAVINGS_PERMANENT_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_P3A_BANDWIDTH_SAVINGS_PERMANENT_STATE_H_

#include <list>

#include "base/macros.h"
#include "base/timer/timer.h"

class PrefService;

namespace brave_perf_predictor {

// Note: append-only enumeration! Never remove any existing values, as this enum
// is used to bucket a UMA histogram, and removing values breaks that.
constexpr std::array<uint64_t, 7> BandwidthSavingsBuckets{
    0,    // 0
    50,   // >0-50mb
    100,  // 51-100mb
    200,  // 101-200mb
    400,  // 201-400mb
    700,  // 401-700mb
    1500  // 701-1500mb
          // >1501 => bucket 7
};

constexpr size_t kNumOfSavedDailyUptimes = 7;
constexpr char kSavingsDailyUMAHistogramName[] =
    "Brave.Savings.BandwidthSavingsMB";

class P3ABandwidthSavingsPermanentState {
 public:
  explicit P3ABandwidthSavingsPermanentState(PrefService* user_prefs);
  ~P3ABandwidthSavingsPermanentState();

  void AddSavings(uint64_t delta);
  uint64_t GetSavingsTotal() const;

 private:
  struct DailySaving {
    base::Time day;
    uint64_t saving;
  };
  void LoadSavingsDaily();
  void SaveSavingsDaily();
  void RecordSavingsTotal();

  std::list<DailySaving> daily_savings_;
  PrefService* user_prefs_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(P3ABandwidthSavingsPermanentState);
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_P3A_BANDWIDTH_SAVINGS_PERMANENT_STATE_H_
