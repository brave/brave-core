/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/state_migration_v11.h"

#include "base/check.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"

namespace ledger {
namespace state {

StateMigrationV11::StateMigrationV11(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

StateMigrationV11::~StateMigrationV11() = default;

void StateMigrationV11::Migrate(ResultCallback callback) {
  // In version 7 encryption was added for |kWalletBrave|. However due to wallet
  // corruption, users copying their profiles to new computers or reinstalling
  // their operating system we are reverting this change

  const auto decrypted_wallet =
      ledger_->state()->GetEncryptedString(kWalletBrave);
  if (decrypted_wallet) {
    ledger_->ledger_client()->SetStringState(kWalletBrave,
                                             decrypted_wallet.value());
  }

  callback(type::Result::LEDGER_OK);
}

}  // namespace state
}  // namespace ledger
