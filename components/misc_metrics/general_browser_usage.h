/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_GENERAL_BROWSER_USAGE_H_
#define BRAVE_COMPONENTS_MISC_METRICS_GENERAL_BROWSER_USAGE_H_

#include <memory>

#include "base/timer/timer.h"

class PrefRegistrySimple;
class PrefService;
class ISOWeeklyStorage;

namespace misc_metrics {

extern const char kWeeklyUseHistogramName[];

class GeneralBrowserUsage {
 public:
  explicit GeneralBrowserUsage(PrefService* local_state);
  ~GeneralBrowserUsage();

  GeneralBrowserUsage(const GeneralBrowserUsage&) = delete;
  GeneralBrowserUsage& operator=(const GeneralBrowserUsage&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  void Update();

  std::unique_ptr<ISOWeeklyStorage> usage_storage_;

  base::RepeatingTimer report_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_GENERAL_BROWSER_USAGE_H_
