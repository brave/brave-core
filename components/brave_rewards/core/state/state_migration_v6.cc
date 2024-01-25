/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state_migration_v6.h"

#include <map>
#include <string>
#include <utility>

#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

namespace brave_rewards::internal {
namespace state {

StateMigrationV6::StateMigrationV6(RewardsEngineImpl& engine)
    : engine_(engine) {}

StateMigrationV6::~StateMigrationV6() = default;

void StateMigrationV6::Migrate(ResultCallback callback) {
  auto uphold_wallet = engine_->GetLegacyWallet();
  engine_->SetState(kWalletUphold, uphold_wallet);
  engine_->client()->ClearState("external_wallets");

  base::Value::Dict brave;
  brave.Set("payment_id", engine_->GetState<std::string>(kPaymentId));
  brave.Set("recovery_seed", engine_->GetState<std::string>(kRecoverySeed));

  std::string brave_json;
  base::JSONWriter::Write(brave, &brave_json);
  engine_->SetState(kWalletBrave, std::move(brave_json));

  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace state
}  // namespace brave_rewards::internal
