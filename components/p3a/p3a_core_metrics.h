/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_P3A_CORE_METRICS_H_
#define BRAVE_COMPONENTS_P3A_P3A_CORE_METRICS_H_

#include "base/timer/timer.h"
#include "chrome/browser/resource_coordinator/usage_clock.h"
#include "chrome/browser/ui/browser_list_observer.h"

class PrefService;
class PrefRegistrySimple;

namespace brave {

class BraveUptimeTracker {
 public:
  explicit BraveUptimeTracker(PrefService* local_state);

 private:
  resource_coordinator::UsageClock usage_clock_;
  DISALLOW_COPY_AND_ASSIGN(BraveUptimeTracker);
};

// Periodically records P3A stats (extracted from Local State) regarding the
// time when incognito windows were used.
// Used as a leaking singletone.
class BraveWindowsTracker : public BrowserListObserver {
 public:
  explicit BraveWindowsTracker(PrefService* local_state);
  ~BraveWindowsTracker() override;

  static void CreateInstance(PrefService* local_state);

  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserSetLastActive(Browser* browser) override;

  void UpdateP3AValues() const;

  base::RepeatingTimer timer_;
  PrefService* local_state_;
  DISALLOW_COPY_AND_ASSIGN(BraveWindowsTracker);
};

// TODO(iefremov): Move to separate *.h/*.cc
// Set callbacks for existing Chromium histograms that will be braveized,
// i.e. reemitted using a different name and custom buckets.
void SetupHistogramsBraveization();

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_P3A_CORE_METRICS_H_
