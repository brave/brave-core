/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CREATE_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CREATE_TRANSACTION_TASK_H_

#include <map>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {
class CardanoWalletService;
class CardanoTransaction;

// This class implements `CardanoWalletService::CreateTransaction` logic of
// creating a Cardano transaction based on wallet's account, destination address
// and amount of native coins to send. Fetches latest block and epoch
// parameters, utxos associated with account . Searches for best utxo set to
// minimize fee. Responds with to-be-signed transaction to `callback_`.
class CardanoCreateTransactionTask {
 public:
  using UtxoMap = std::map<CardanoAddress, cardano_rpc::UnspentOutputs>;
  using Callback =
      base::OnceCallback<void(base::expected<CardanoTransaction, std::string>)>;

  CardanoCreateTransactionTask(CardanoWalletService& cardano_wallet_service,
                               const mojom::AccountIdPtr& account_id,
                               const CardanoAddress& address_to,
                               uint64_t amount,
                               bool sending_max_amount);

  ~CardanoCreateTransactionTask();

  void Start(Callback callback);

 private:
  CardanoTransaction::TxOutput CreateTargetOutput();
  CardanoTransaction::TxOutput CreateChangeOutput();

  void ScheduleWorkOnTask();

  void WorkOnTask();
  void StopWithError(std::string error_string);

  void OnGetLatestEpochParameters(base::expected<cardano_rpc::EpochParameters,
                                                 std::string> epoch_parameters);
  void OnGetLatestBlock(base::expected<cardano_rpc::Block, std::string> bock);
  void OnGetUtxos(base::expected<UtxoMap, std::string> utxos);
  void OnDiscoverNextUnusedChangeAddress(
      base::expected<mojom::CardanoAddressPtr, std::string> address);

  raw_ref<CardanoWalletService> cardano_wallet_service_;
  mojom::AccountIdPtr account_id_;
  CardanoAddress address_to_;
  bool sending_max_amount_ = false;

  CardanoTransaction transaction_;
  mojom::CardanoAddressPtr change_address_;

  std::optional<cardano_rpc::EpochParameters> latest_epoch_parameters_;
  std::optional<cardano_rpc::Block> latest_block_;

  bool has_solved_transaction_ = false;
  std::optional<std::map<CardanoAddress, cardano_rpc::UnspentOutputs>>
      utxo_map_;
  Callback callback_;
  base::WeakPtrFactory<CardanoCreateTransactionTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CREATE_TRANSACTION_TASK_H_
