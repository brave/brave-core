/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_internals_info.h"

namespace brave_rewards {

RewardsInternalsInfo::RewardsInternalsInfo() {}

RewardsInternalsInfo::RewardsInternalsInfo(const RewardsInternalsInfo& info)
    : payment_id(info.payment_id),
      is_key_info_seed_valid(info.is_key_info_seed_valid),
      current_reconciles(info.current_reconciles) {
}

RewardsInternalsInfo::~RewardsInternalsInfo() {}

}  // namespace brave_rewards
