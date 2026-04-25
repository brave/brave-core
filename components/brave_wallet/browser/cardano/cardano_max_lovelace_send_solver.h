/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_MAX_LOVELACE_SEND_SOLVER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_MAX_LOVELACE_SEND_SOLVER_H_

#include <string>
#include <vector>

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"

namespace brave_wallet {

class CardanoMaxLovelaceSendSolver {
 public:
  CardanoMaxLovelaceSendSolver(TxBuilderParms builder_params,
                               std::vector<CardanoTransaction::TxInput> inputs);
  ~CardanoMaxLovelaceSendSolver();

  // Searches for best transaction(lesser fee).
  base::expected<CardanoTransaction, std::string> Solve();

 private:
  FRIEND_TEST_ALL_PREFIXES(CardanoMaxLovelaceSendSolverUnitTest, SetupOutputs);

  // If any of inputs have tokens attached to corresponding utxos make sure
  // there is a valid change output having these tokens set to it.
  static bool SetupOutputs(CardanoTransaction& tx,
                           const TxBuilderParms& builder_params);

  // Various params required to create transaction.
  TxBuilderParms builder_params_;
  // Set of possible inputs to pick for transaction.
  std::vector<CardanoTransaction::TxInput> inputs_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_MAX_LOVELACE_SEND_SOLVER_H_
