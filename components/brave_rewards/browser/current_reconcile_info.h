/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CURRENT_RECONCILE_INFO_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CURRENT_RECONCILE_INFO_

#include <map>
#include <string>

namespace brave_rewards {

enum ContributionRetry {
  STEP_NO = 0,
  STEP_RECONCILE = 1,  // Phase 1
  STEP_CURRENT = 2,    // Phase 1
  STEP_PAYLOAD = 3,    // Phase 1
  STEP_REGISTER = 4,   // Phase 1
  STEP_VIEWING = 5,    // Phase 1
  STEP_WINNERS = 6,    // Phase 1
  STEP_PREPARE = 7,    // Phase 2
  STEP_PROOF = 8,      // Phase 2
  STEP_VOTE = 9,       // Phase 2
  STEP_FINAL = 10      // Phase 2
};

struct CurrentReconcileInfo {
  CurrentReconcileInfo();
  ~CurrentReconcileInfo();
  CurrentReconcileInfo(const CurrentReconcileInfo& info);

  std::string viewing_id_;
  std::string amount_;
  ContributionRetry retry_step_;
  int retry_level_;
};

}  // namespace brave_rewards

#endif //BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CURRENT_RECONCILE_INFO_
