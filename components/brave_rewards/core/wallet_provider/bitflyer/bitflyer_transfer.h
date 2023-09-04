/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_BITFLYER_BITFLYER_TRANSFER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_BITFLYER_BITFLYER_TRANSFER_H_

#include "brave/components/brave_rewards/core/endpoints/bitflyer/post_commit_transaction_bitflyer.h"
#include "brave/components/brave_rewards/core/wallet_provider/transfer.h"

namespace brave_rewards::internal::bitflyer {

class BitFlyerTransfer final : public wallet_provider::Transfer {
 public:
  using Transfer::Transfer;

 private:
  void CommitTransaction(ResultCallback,
                         mojom::ExternalTransactionPtr) const override;

  void OnCommitTransaction(
      ResultCallback,
      endpoints::PostCommitTransactionBitFlyer::Result&&) const;
};

}  // namespace brave_rewards::internal::bitflyer

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_BITFLYER_BITFLYER_TRANSFER_H_
