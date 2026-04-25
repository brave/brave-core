/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_MAX_TOKEN_SEND_SOLVER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_MAX_TOKEN_SEND_SOLVER_H_

#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"

namespace brave_wallet {

class CardanoMaxTokenSendSolver {
 public:
  CardanoMaxTokenSendSolver(TxBuilderParms builder_params,
                            std::vector<CardanoTransaction::TxInput> inputs);
  ~CardanoMaxTokenSendSolver();

  // Searches for best transaction(lesser fee).
  base::expected<CardanoTransaction, std::string> Solve();

 private:
  FRIEND_TEST_ALL_PREFIXES(CardanoMaxTokenSendSolverUnitTest, SetupOutputs);
  FRIEND_TEST_ALL_PREFIXES(CardanoMaxTokenSendSolverUnitTest,
                           ExtractTokenInputs);
  FRIEND_TEST_ALL_PREFIXES(CardanoMaxTokenSendSolverUnitTest, SortInputs);

  // Setup outputs for tx to send all inputs with a given token.
  // Target output gets all amount of given token and some min lovelace to cover
  // fee.
  // Change output gets everything else.
  static bool SetupOutputs(CardanoTransaction& tx,
                           const TxBuilderParms& builder_params);

  // Split inputs into two collections. First one with inputs having given
  // token. All of these inputs will be added to the transaction. Second
  // ones gets everything else. Will be used if transaction needs
  // more lovelaces to cover fee.
  static std::pair<std::vector<CardanoTransaction::TxInput>,
                   std::vector<CardanoTransaction::TxInput>>
  SplitInputsByToken(const cardano_rpc::TokenId& token_id,
                     std::vector<CardanoTransaction::TxInput> inputs);

  // Sort inputs so it is preferred to pick inputs with less tokens with higher
  // lovelace amount.
  static void SortInputsBySelectionPriority(
      std::vector<CardanoTransaction::TxInput>& inputs);

  // Various params required to create transaction.
  TxBuilderParms builder_params_;
  // Set of possible inputs to pick for transaction.
  std::vector<CardanoTransaction::TxInput> inputs_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_MAX_TOKEN_SEND_SOLVER_H_
