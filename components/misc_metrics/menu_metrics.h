/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_MENU_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_MENU_METRICS_H_

#include "base/containers/flat_map.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/timer/timer.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class PrefService;

namespace misc_metrics {

enum class MenuGroup {
  kTabWindow,
  kBraveFeatures,
  kBrowserViews,
};

extern const char kFrequentMenuGroupHistogramName[];

class MenuMetrics {
 public:
  explicit MenuMetrics(PrefService* local_state);
  ~MenuMetrics();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordMenuGroupAction(MenuGroup group);

 private:
  base::flat_map<MenuGroup, double> menu_group_access_counts_;

  raw_ptr<PrefService> local_state_ = nullptr;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_MENU_METRICS_H_
