/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_P3A_BANDWIDTH_SAVINGS_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_P3A_BANDWIDTH_SAVINGS_TRACKER_H_

#include <array>
#include <list>

#include "base/macros.h"
#include "base/timer/timer.h"

class PrefRegistrySimple;
class PrefService;

namespace brave_perf_predictor {

class P3ABandwidthSavingsTracker {
 public:
  explicit P3ABandwidthSavingsTracker(PrefService* user_prefs);
  ~P3ABandwidthSavingsTracker();
  static void RegisterPrefs(PrefRegistrySimple* registry);
  void RecordSavings(uint64_t savings);

 private:
  PrefService* user_prefs_;

  DISALLOW_COPY_AND_ASSIGN(P3ABandwidthSavingsTracker);
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_P3A_BANDWIDTH_SAVINGS_TRACKER_H_
