/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_MAX_SEND_SOLVER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_MAX_SEND_SOLVER_H_

#include <string>
#include <vector>

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"

namespace brave_wallet {

class CardanoMaxSendSolver {
 public:
  CardanoMaxSendSolver(CardanoTransaction base_transaction,
                       CardanoAddress change_address,
                       cardano_rpc::EpochParameters latest_epoch_parameters,
                       std::vector<CardanoTransaction::TxInput> inputs);
  ~CardanoMaxSendSolver();

  // Searches for best transaction(lesser fee).
  base::expected<CardanoTransaction, std::string> Solve();

 private:
  // Initial transaction we are trying to find inputs for.
  CardanoTransaction base_transaction_;
  // Change address in case we need to send change.
  CardanoAddress change_address_;
  // Current state of blockchain. Used to calculate fee.
  cardano_rpc::EpochParameters latest_epoch_parameters_;
  // Set of possible inputs to pick for transaction.
  std::vector<CardanoTransaction::TxInput> inputs_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_MAX_SEND_SOLVER_H_
