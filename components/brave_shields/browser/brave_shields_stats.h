/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_STATS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_STATS_H_

#include "base/macros.h"
#include "base/memory/singleton.h"

namespace brave_shields {

class BraveShieldsStats {
 public:
  static BraveShieldsStats* GetInstance();
  void IncrementAdsBlocked();
  void IncrementTrackersBlocked();
  void IncrementJavascriptBlocked();
  void IncrementHttpsUpgrades();
  void IncrementFingerprintingBlocked();

 protected:
  BraveShieldsStats();
  ~BraveShieldsStats();
  friend struct base::DefaultSingletonTraits<BraveShieldsStats>;

  int64_t ads_blocked_;
  int64_t trackers_blocked_;
  int64_t javascript_blocked_;
  int64_t https_upgrades_;
  int64_t fingerprinting_blocked_;

 protected:
  DISALLOW_COPY_AND_ASSIGN(BraveShieldsStats);
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_SHIELDS_STATS_H_

