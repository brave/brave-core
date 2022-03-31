/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_STATS_BROWSER_BRAVE_STATS_UPDATER_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_STATS_BROWSER_BRAVE_STATS_UPDATER_OBSERVER_H_

#include "base/observer_list_types.h"

namespace brave_stats {

class BraveStatsUpdaterObserver : public base::CheckedObserver {
 public:
  ~BraveStatsUpdaterObserver() override {}

  virtual void OnStatsPingFired() {}
};

}  // namespace brave_stats
#endif  // BRAVE_COMPONENTS_BRAVE_STATS_BROWSER_BRAVE_STATS_UPDATER_OBSERVER_H_
