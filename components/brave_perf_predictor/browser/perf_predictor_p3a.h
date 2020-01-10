/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_P3A_H_

#include <array>
#include <list>

#include "base/macros.h"
#include "base/timer/timer.h"

#include "brave/components/brave_perf_predictor/browser/perf_predictor_p3a.h"

class PrefRegistrySimple;
class PrefService;

namespace brave_perf_predictor {

// Note: append-only enumeration! Never remove any existing values, as this enum
// is used to bucket a UMA histogram, and removing values breaks that.
constexpr std::array<uint64_t, 7> BandwidthSavingsBuckets {
  0,  // 0
  50, // >0-50mb
  100, // 51-100mb
  200, // 101-200mb
  400, // 201-400mb
  700, // 401-700mb
  1500 // 701-1500mb
  // >1501 => bucket 7
};

constexpr size_t kNumOfSavedDailyUptimes = 7;
constexpr char kSavingsDailyListPrefName[] = "brave_perf_predictor_daily_savings";
constexpr char kSavingsDailyUMAHistogramName[] = "Brave.Savings.BandwidthSavingsMB";

class SavingPermanentState {
 public:
  explicit SavingPermanentState(PrefService* local_state);
  ~SavingPermanentState();

  void AddSaving(uint64_t delta);
  uint64_t GetTotalSaving() const;

 private:
  struct DailySaving {
    base::Time day;
    uint64_t saving;
  };
  void LoadSavings();
  void SaveSavings();
  void RecordP3A();

  std::list<DailySaving> daily_savings_;
  PrefService* local_state_ = nullptr;
};

class BandwidthSavingsTracker {
 public:
  explicit BandwidthSavingsTracker(PrefService* local_state);
  ~BandwidthSavingsTracker();
  static void RegisterPrefs(PrefRegistrySimple* registry);
  void RecordSaving(uint64_t saving);

 private:
  PrefService* local_state_;

  DISALLOW_COPY_AND_ASSIGN(BandwidthSavingsTracker);
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_P3A_H_
