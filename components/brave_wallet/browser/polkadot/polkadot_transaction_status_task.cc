/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_transaction_status_task.h"

#include "base/strings/strcat.h"  // IWYU pragma: export

namespace brave_wallet {

PolkadotTransactionStatusTask::PolkadotTransactionStatusTask(
    PolkadotWalletService& polkadot_wallet_service,
    KeyringService& keyring_service,
    mojom::AccountIdPtr sender_account_id,
    std::string chain_id,
    std::vector<uint8_t> extrinsic,
    uint32_t block_num,
    uint32_t mortality_period)
    : polkadot_wallet_service_(polkadot_wallet_service),
      keyring_service_(keyring_service),
      sender_account_id_(std::move(sender_account_id)),
      chain_id_(std::move(chain_id)),
      extrinsic_hex_(base::StrCat({"0x", base::HexEncodeLower(extrinsic)})),
      block_num_{block_num},
      curr_block_num_{block_num},
      mortality_period_{mortality_period} {}

PolkadotTransactionStatusTask::~PolkadotTransactionStatusTask() {}

}  // namespace brave_wallet
