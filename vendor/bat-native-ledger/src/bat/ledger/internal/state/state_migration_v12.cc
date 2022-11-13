/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/state_migration_v12.h"

#include <utility>
#include <vector>

#include "base/ranges/algorithm.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/wallet/wallet_util.h"

namespace ledger::state {

StateMigrationV12::StateMigrationV12(LedgerImpl* ledger) : ledger_(ledger) {
  DCHECK(ledger_);
}

StateMigrationV12::~StateMigrationV12() = default;

// |WalletStatus| definition pre-v12:
// enum WalletStatus {
//   NOT_CONNECTED = 0,
//   CONNECTED = 1,
//   VERIFIED = 2,
//   DISCONNECTED_NOT_VERIFIED = 3,
//   DISCONNECTED_VERIFIED = 4,
//   PENDING = 5
// };

// |WalletStatus| definition as of v12:
// enum WalletStatus {
//   kNotConnected = 0,
//   kConnected = 2,
//   kLoggedOut = 4
// };

bool StateMigrationV12::MigrateExternalWallet(const std::string& wallet_type) {
  DCHECK(ledger_);
  if (!ledger_) {
    BLOG(0, "ledger_ is null!");
    return false;
  }

  auto wallet = wallet::GetWallet(ledger_, wallet_type);
  if (!wallet) {
    BLOG(1, "User doesn't have a(n) " << wallet_type << " wallet.");
    return true;
  }

  switch (const auto status =
              static_cast<std::underlying_type_t<mojom::WalletStatus>>(
                  wallet->status)) {
    case 0:  // pre-v12 NOT_CONNECTED ==> v12 kNotConnected
      wallet->status = mojom::WalletStatus::kNotConnected;
      wallet->token = "";
      wallet->address = "";
      break;
    case 2:  // pre-v12 VERIFIED ==> v12 (kConnected || kLoggedOut)
      if (!wallet->token.empty() && !wallet->address.empty()) {
        wallet->status = mojom::WalletStatus::kConnected;
      } else {
        wallet->status = mojom::WalletStatus::kLoggedOut;
        wallet->token = "";
        wallet->address = "";
      }

      break;
    case 4:  // pre-v12 DISCONNECTED_VERIFIED ==> v12 kLoggedOut
      wallet->status = mojom::WalletStatus::kLoggedOut;
      wallet->token = "";
      wallet->address = "";
      break;
    case 1:  // pre-v12 CONNECTED ==> v12 kNotConnected
    case 3:  // pre-v12 DISCONNECTED_NOT_VERIFIED ==> v12 kNotConnected
    case 5:  // pre-v12 PENDING ==> v12 kNotConnected
      wallet->status = mojom::WalletStatus::kNotConnected;
      wallet->token = "";
      wallet->address = "";

      break;
    default:
      NOTREACHED() << "Unexpected " << wallet_type
                   << " wallet status: " << status << '!';
      return false;
  }

  wallet->one_time_string = util::GenerateRandomHexString();
  wallet->code_verifier = util::GeneratePKCECodeVerifier();
  wallet = wallet::GenerateLinks(std::move(wallet));
  if (!wallet) {
    BLOG(0, "Failed to generate links for " << wallet_type << " wallet!");
    return false;
  }

  if (!wallet::SetWallet(ledger_, std::move(wallet))) {
    BLOG(0, "Failed to set " << wallet_type << " wallet!");
    return false;
  }

  return true;
}

void StateMigrationV12::Migrate(ledger::LegacyResultCallback callback) {
  callback(base::ranges::all_of(
               std::vector{constant::kWalletBitflyer, constant::kWalletGemini,
                           constant::kWalletUphold},
               [this](const std::string& wallet_type) {
                 return MigrateExternalWallet(wallet_type);
               })
               ? mojom::Result::LEDGER_OK
               : mojom::Result::LEDGER_ERROR);
}

}  // namespace ledger::state
