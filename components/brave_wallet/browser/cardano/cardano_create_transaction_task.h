/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CREATE_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CREATE_TRANSACTION_TASK_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_responses.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {
class CardanoWalletService;
class CardanoTransaction;

class CreateCardanoTransactionTask {
 public:
  using UtxoMap = std::map<std::string, cardano_rpc::UnspentOutputs>;
  using Callback =
      base::OnceCallback<void(base::expected<CardanoTransaction, std::string>)>;

  CreateCardanoTransactionTask(
      base::WeakPtr<CardanoWalletService> cardano_wallet_service,
      const mojom::AccountIdPtr& account_id,
      const std::string& address_to,
      uint64_t amount,
      bool sending_max_amount);

  virtual ~CreateCardanoTransactionTask();

  void set_callback(Callback callback) { callback_ = std::move(callback); }

  void ScheduleWorkOnTask();

 private:
  void SetError(const std::string& error_string) { error_ = error_string; }

  CardanoTransaction::TxOutput CreateTargetOutput();
  CardanoTransaction::TxOutput CreateChangeOutput();

  void WorkOnTask();

  void OnGetLatestEpochParameters(base::expected<cardano_rpc::EpochParameters,
                                                 std::string> epoch_parameters);
  void OnGetUtxos(base::expected<UtxoMap, std::string> utxos);

  base::WeakPtr<CardanoWalletService> cardano_wallet_service_;
  mojom::AccountIdPtr account_id_;
  std::string address_to_;
  uint64_t amount_ = 0;
  bool sending_max_amount_ = false;

  CardanoTransaction transaction_;
  mojom::CardanoAddressPtr change_address_;

  std::optional<cardano_rpc::EpochParameters> latest_epoch_parameters_;

  bool has_solved_transaction_ = false;
  std::map<std::string, cardano_rpc::UnspentOutputs> utxo_map_;
  std::optional<std::string> error_;
  Callback callback_;
  base::WeakPtrFactory<CreateCardanoTransactionTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_CREATE_TRANSACTION_TASK_H_
