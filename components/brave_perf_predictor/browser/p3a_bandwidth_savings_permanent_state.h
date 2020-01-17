/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_P3A_BANDWIDTH_SAVINGS_PERMANENT_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_P3A_BANDWIDTH_SAVINGS_PERMANENT_STATE_H_

#include <list>

#include "base/time/time.h"
#include "base/values.h"

class PrefService;

namespace brave_perf_predictor {

// This class accumulates savings reported via |AddSavings| over time in
// |PrefService| User Preferences for persistency and returns those for the last
// full period available when queried via |GetFullPeriodSavingsBytes|.
//
// Time interval to accumulate data for is defined internally and
// |GetFullPeriodSavingsBytes| returns 0 if there aren't enough readings to
// cover a full period.
class P3ABandwidthSavingsPermanentState {
 public:
  explicit P3ABandwidthSavingsPermanentState(PrefService* user_prefs);
  ~P3ABandwidthSavingsPermanentState();
  P3ABandwidthSavingsPermanentState(const P3ABandwidthSavingsPermanentState&) =
      delete;
  P3ABandwidthSavingsPermanentState& operator=(
      const P3ABandwidthSavingsPermanentState&) = delete;

  void AddSavings(uint64_t delta);
  base::Optional<uint64_t> GetFullPeriodSavingsBytes();

 private:
  struct DailySaving {
    base::Time day;
    uint64_t saving;
    // DailySaving(base::Time day, uint64_t saving): day(day), saving(saving) {}
  };
  void LoadSavingsDaily();
  void SaveSavingsDaily();
  void RecordSavingsTotal();
  uint64_t GetSavingsTotal() const;

  std::list<DailySaving> daily_savings_;
  PrefService* user_prefs_ = nullptr;
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_P3A_BANDWIDTH_SAVINGS_PERMANENT_STATE_H_
