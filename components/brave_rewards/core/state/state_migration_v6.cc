/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state_migration_v6.h"

#include <map>
#include <string>
#include <utility>

#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

namespace ledger {
namespace state {

StateMigrationV6::StateMigrationV6(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

StateMigrationV6::~StateMigrationV6() = default;

void StateMigrationV6::Migrate(ledger::LegacyResultCallback callback) {
  auto uphold_wallet = ledger_->GetLegacyWallet();
  ledger_->SetState(kWalletUphold, uphold_wallet);
  ledger_->client()->ClearState("external_wallets");

  base::Value::Dict brave;
  brave.Set("payment_id", ledger_->GetState<std::string>(kPaymentId));
  brave.Set("recovery_seed", ledger_->GetState<std::string>(kRecoverySeed));

  std::string brave_json;
  base::JSONWriter::Write(brave, &brave_json);
  ledger_->SetState(kWalletBrave, brave_json);

  callback(mojom::Result::LEDGER_OK);
}

}  // namespace state
}  // namespace ledger
