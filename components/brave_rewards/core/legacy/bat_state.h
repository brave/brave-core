/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_BAT_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_BAT_STATE_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/legacy/client_properties.h"
#include "brave/components/brave_rewards/core/legacy/wallet_info_properties.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {

class RewardsEngineImpl;

class LegacyBatState {
 public:
  explicit LegacyBatState(RewardsEngineImpl& engine);
  ~LegacyBatState();

  void Load(LegacyResultCallback callback);

  bool GetRewardsMainEnabled() const;

  double GetAutoContributionAmount() const;

  bool GetUserChangedContribution() const;

  bool GetAutoContributeEnabled() const;

  const std::string& GetCardIdAddress() const;

  uint64_t GetReconcileStamp() const;

  const std::string& GetPaymentId() const;

  const std::vector<uint8_t>& GetRecoverySeed() const;

  uint64_t GetCreationStamp() const;

 private:
  void OnLoad(LegacyResultCallback callback,
              mojom::Result result,
              const std::string& data);

  const raw_ref<RewardsEngineImpl> engine_;
  ClientProperties state_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_BAT_STATE_H_
