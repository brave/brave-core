/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state_migration_v10.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace brave_rewards::internal {
namespace state {

StateMigrationV10::StateMigrationV10(RewardsEngineImpl& engine)
    : engine_(engine),
      get_wallet_{std::make_unique<endpoint::promotion::GetWallet>(engine)} {}

StateMigrationV10::~StateMigrationV10() = default;

// mojom::WalletStatus::CONNECTED (1),
// mojom::WalletStatus::DISCONNECTED_NOT_VERIFIED (3), and
// mojom::WalletStatus::PENDING (5) have been removed.

// mojom::WalletStatus::NOT_CONNECTED (0) has been renamed to
// mojom::WalletStatus::kNotConnected (0),
// mojom::WalletStatus::VERIFIED (2) has been renamed to
// mojom::WalletStatus::kConnected (2), and
// mojom::WalletStatus::DISCONNECTED_VERIFIED (4) has been renamed to
// mojom::WalletStatus::kLoggedOut (4).

void StateMigrationV10::Migrate(LegacyResultCallback callback) {
  auto uphold_wallet = engine_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(1, "Uphold wallet is null.");
    return callback(mojom::Result::OK);
  }

  switch (static_cast<std::underlying_type_t<mojom::WalletStatus>>(
      uphold_wallet->status)) {
    case 0:  // mojom::WalletStatus::NOT_CONNECTED
      uphold_wallet->token = "";
      uphold_wallet->address = "";
      break;
    case 1:  // mojom::WalletStatus::CONNECTED
      uphold_wallet->status = static_cast<mojom::WalletStatus>(
          !uphold_wallet->token.empty()
              ? 5    // mojom::WalletStatus::PENDING
              : 0);  // mojom::WalletStatus::NOT_CONNECTED
      uphold_wallet->address = "";
      break;
    case 2: {  // mojom::WalletStatus::VERIFIED
      if (uphold_wallet->token.empty() || uphold_wallet->address.empty()) {
        uphold_wallet->status = static_cast<mojom::WalletStatus>(
            !uphold_wallet->token.empty()
                ? 5    // mojom::WalletStatus::PENDING
                : 4);  // mojom::WalletStatus::DISCONNECTED_VERIFIED
        uphold_wallet->address = "";
        break;
      }

      auto wallet_info_endpoint_callback = std::bind(
          &StateMigrationV10::OnGetWallet, this, _1, _2, _3, callback);

      if (is_testing) {
        return wallet_info_endpoint_callback(mojom::Result::FAILED,
                                             std::string{}, false);
      } else {
        return get_wallet_->Request(std::move(wallet_info_endpoint_callback));
      }
    }
    case 3:  // mojom::WalletStatus::DISCONNECTED_NOT_VERIFIED
      uphold_wallet->status = static_cast<mojom::WalletStatus>(
          4);  // mojom::WalletStatus::DISCONNECTED_VERIFIED
      uphold_wallet->token = "";
      uphold_wallet->address = "";
      break;
    case 4:  // mojom::WalletStatus::DISCONNECTED_VERIFIED
      uphold_wallet->token = "";
      uphold_wallet->address = "";
      break;
    case 5:  // mojom::WalletStatus::PENDING
      uphold_wallet->status = static_cast<mojom::WalletStatus>(
          !uphold_wallet->token.empty()
              ? 5    // mojom::WalletStatus::PENDING
              : 0);  // mojom::WalletStatus::NOT_CONNECTED
      uphold_wallet->address = "";
      break;
    default:
      NOTREACHED();
  }

  uphold_wallet = uphold::GenerateLinks(std::move(uphold_wallet));
  callback(engine_->uphold()->SetWallet(std::move(uphold_wallet))
               ? mojom::Result::OK
               : mojom::Result::FAILED);
}

void StateMigrationV10::OnGetWallet(mojom::Result result,
                                    const std::string& custodian,
                                    bool linked,
                                    LegacyResultCallback callback) {
  auto uphold_wallet = engine_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(mojom::Result::FAILED);
  }

  DCHECK(uphold_wallet->status ==
         static_cast<mojom::WalletStatus>(2));  // mojom::WalletStatus::VERIFIED
  DCHECK(!uphold_wallet->token.empty());
  DCHECK(!uphold_wallet->address.empty());

  if (result != mojom::Result::OK || custodian != constant::kWalletUphold ||
      !linked) {  // deemed semi-VERIFIED || semi-VERIFIED
    uphold_wallet->status =
        static_cast<mojom::WalletStatus>(5);  // mojom::WalletStatus::PENDING
    uphold_wallet->address = "";
  }

  uphold_wallet = uphold::GenerateLinks(std::move(uphold_wallet));
  callback(engine_->uphold()->SetWallet(std::move(uphold_wallet))
               ? mojom::Result::OK
               : mojom::Result::FAILED);
}

}  // namespace state
}  // namespace brave_rewards::internal
