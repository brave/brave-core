/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_INTERNALS_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_INTERNALS_INFO_H_

#include <map>
#include <string>

#include "brave/components/brave_rewards/browser/current_reconcile_info.h"

namespace brave_rewards {

struct RewardsInternalsInfo {
  RewardsInternalsInfo();
  ~RewardsInternalsInfo();
  RewardsInternalsInfo(const RewardsInternalsInfo& info);

  std::string payment_id;
  bool is_key_info_seed_valid;

  std::map<std::string, CurrentReconcileInfo> current_reconciles;
};

}  // namespace brave_rewards

#endif //BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_INTERNALS_INFO_H_
