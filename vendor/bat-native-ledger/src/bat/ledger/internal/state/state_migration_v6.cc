/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/state_migration_v6.h"

#include <map>
#include <string>
#include <utility>

#include "base/json/json_writer.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"

namespace ledger {
namespace state {

StateMigrationV6::StateMigrationV6(LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

StateMigrationV6::~StateMigrationV6() = default;

void StateMigrationV6::Migrate(ledger::ResultCallback callback) {
  auto uphold_wallet = ledger_->ledger_client()->GetLegacyWallet();
  ledger_->ledger_client()->SetStringState(
      kWalletUphold,
      uphold_wallet);
  ledger_->ledger_client()->ClearState("external_wallets");

  base::Value brave(base::Value::Type::DICTIONARY);
  brave.SetKey(
      "payment_id",
      base::Value(ledger_->ledger_client()->GetStringState(kPaymentId)));
  brave.SetKey(
      "recovery_seed",
      base::Value(ledger_->ledger_client()->GetStringState(kRecoverySeed)));

  std::string brave_json;
  base::JSONWriter::Write(brave, &brave_json);
  ledger_->ledger_client()->SetStringState(
      kWalletBrave,
      brave_json);


  callback(type::Result::LEDGER_OK);
}

}  // namespace state
}  // namespace ledger
