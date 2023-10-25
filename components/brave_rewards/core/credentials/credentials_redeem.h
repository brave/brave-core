/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_REDEEM_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_REDEEM_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"

namespace brave_rewards::internal {
namespace credential {

struct CredentialsRedeem {
  CredentialsRedeem();
  CredentialsRedeem(const CredentialsRedeem& info);
  ~CredentialsRedeem();

  std::string publisher_key;
  mojom::RewardsType type;
  mojom::ContributionProcessor processor;
  std::vector<mojom::UnblindedToken> token_list;
  std::string order_id;
  std::string contribution_id;
};

}  // namespace credential
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_REDEEM_H_
