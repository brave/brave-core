/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/uphold_server.h"
#include "brave/components/brave_rewards/core/wallet_provider/wallet_provider.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace uphold {

class Uphold final : public wallet_provider::WalletProvider {
 public:
  explicit Uphold(RewardsEngineImpl& engine);

  const char* WalletType() const override;

  void FetchBalance(
      base::OnceCallback<void(mojom::Result, double)> callback) override;

  std::string GetFeeAddress() const override;

 private:
  endpoint::UpholdServer server_;
};

}  // namespace uphold
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_UPHOLD_UPHOLD_H_
