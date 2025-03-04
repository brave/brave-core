/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_create_transaction_task.h"

#include <stdint.h>

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_knapsack_solver.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

namespace {
std::vector<CardanoTransaction::TxInput> TxInputsFromUtxoMap(
    const std::map<std::string, cardano_rpc::UnspentOutputs>& map) {
  std::vector<CardanoTransaction::TxInput> result;
  for (auto& [key, value] : map) {
    for (auto& utxo : value) {
      if (auto input = CardanoTransaction::TxInput::FromRpcUtxo(key, utxo)) {
        result.push_back(std::move(*input));
      }
    }
  }

  return result;
}
}  // namespace

CreateCardanoTransactionTask::CreateCardanoTransactionTask(
    base::WeakPtr<CardanoWalletService> cardano_wallet_service,
    const mojom::AccountIdPtr& account_id,
    const std::string& address_to,
    uint64_t amount,
    bool sending_max_amount)
    : cardano_wallet_service_(cardano_wallet_service),
      account_id_(account_id.Clone()),
      address_to_(address_to),
      amount_(amount),
      sending_max_amount_(sending_max_amount) {}

CreateCardanoTransactionTask::~CreateCardanoTransactionTask() = default;

void CreateCardanoTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CreateCardanoTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

CardanoTransaction::TxOutput
CreateCardanoTransactionTask::CreateTargetOutput() {
  CardanoTransaction::TxOutput target_output;
  target_output.type = CardanoTransaction::TxOutputType::kTarget;
  target_output.amount =
      transaction_.sending_max_amount() ? 0 : transaction_.amount();
  target_output.address = transaction_.to();
  // target_output.script_pubkey = CardanoSerializer::AddressToScriptPubkey(
  //     target_output.address, IsTestnet());
  // CHECK(target_output.script_pubkey.size());

  return target_output;
}

CardanoTransaction::TxOutput
CreateCardanoTransactionTask::CreateChangeOutput() {
  CardanoTransaction::TxOutput change_output;
  change_output.type = CardanoTransaction::TxOutputType::kChange;
  change_output.amount = 0;
  change_output.address = change_address_->address_string;
  // change_output.script_pubkey = CardanoSerializer::AddressToScriptPubkey(
  //     change_output.address, IsTestnet());
  // CHECK(change_output.script_pubkey.size());

  return change_output;
}

void CreateCardanoTransactionTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    return;
  }

  (void)amount_;
  (void)sending_max_amount_;
  (void)has_solved_transaction_;

  if (!latest_epoch_parameters_) {
    cardano_wallet_service_->cardano_rpc().GetLatestEpochParameters(
        GetNetworkForCardanoAccount(account_id_),
        base::BindOnce(
            &CreateCardanoTransactionTask::OnGetLatestEpochParameters,
            weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // if (CardanoSerializer::AddressToScriptPubkey(transaction_.to(),
  // IsTestnet())
  //         .empty()) {
  //   SetError(WalletParsingErrorMessage());
  //   ScheduleWorkOnTask();
  //   return;
  // }

  // if (!chain_height_) {
  //   cardano_wallet_service_->bitcoin_rpc().GetChainHeight(
  //       GetNetworkForCardanoAccount(account_id_),
  //       base::BindOnce(&CreateTransactionTask::OnGetChainHeight,
  //                      weak_ptr_factory_.GetWeakPtr()));
  //   return;
  // }

  if (utxo_map_.empty()) {
    cardano_wallet_service_->GetUtxos(
        account_id_.Clone(),
        base::BindOnce(&CreateCardanoTransactionTask::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // if (!change_address_) {
  //   cardano_wallet_service_->DiscoverNextUnusedAddress(
  //       account_id_.Clone(), true,
  //       base::BindOnce(
  //           &CreateTransactionTask::OnDiscoverNextUnusedChangeAddress,
  //           weak_ptr_factory_.GetWeakPtr()));
  //   return;
  // }

  // TODO(apaymyshev): this captures fee estimates at transaction creation
  // moment and can become outdated when transaction is actually broadcasted
  // to network. Should handle this somehow.
  // if (estimates_.empty()) {
  //   cardano_wallet_service_->bitcoin_rpc().GetFeeEstimates(
  //       GetNetworkForCardanoAccount(account_id_),
  //       base::BindOnce(&CreateTransactionTask::OnGetFeeEstimates,
  //                      weak_ptr_factory_.GetWeakPtr()));
  //   return;
  // }

  // TODO(apaymyshev): random shift locktime
  // https://github.com/bitcoin/bitcoin/blob/v24.0/src/wallet/spend.cpp#L739-L747
  // transaction_.set_locktime(chain_height_.value());

  if (!has_solved_transaction_) {
    base::expected<CardanoTransaction, std::string> solved_transaction;
    // if (transaction_.sending_max_amount()) {
    //   transaction_.AddOutput(CreateTargetOutput());
    //   CardanoMaxSendSolver solver(transaction_, GetFeeRate(),
    //                               TxInputGroupsFromUtxoMap(utxo_map_));
    //   solved_transaction = solver.Solve();
    // } else
    {
      transaction_.AddOutput(CreateTargetOutput());
      transaction_.AddOutput(CreateChangeOutput());

      // TODO(apaymyshev): consider moving this calculation to separate
      CardanoKnapsackSolver solver(
          transaction_, latest_epoch_parameters_->min_fee_a,
          latest_epoch_parameters_->min_fee_b, TxInputsFromUtxoMap(utxo_map_));
      solved_transaction = solver.Solve();
    }

    if (!solved_transaction.has_value()) {
      SetError(solved_transaction.error());
      ScheduleWorkOnTask();
      return;
    }

    has_solved_transaction_ = true;
    transaction_ = std::move(*solved_transaction);
    // if (rearrange_for_testing_) {
    //   transaction_.ArrangeTransactionForTesting();  // IN-TEST
    // } else {
    // transaction_.ShuffleTransaction();
    // }
  }

  // if (ShouldFetchRawTransactions()) {
  //   std::vector<SHA256HashArray> txids;
  //   for (auto& input : transaction_.inputs()) {
  //     txids.push_back(input.utxo_outpoint.txid);
  //   }
  //   cardano_wallet_service_->FetchRawTransactions(
  //       GetNetworkForCardanoAccount(account_id_), txids,
  //       base::BindOnce(&CreateTransactionTask::OnFetchRawTransactions,
  //                      weak_ptr_factory_.GetWeakPtr()));
  //   return;
  // }

  std::move(callback_).Run(base::ok(std::move(transaction_)));
}

void CreateCardanoTransactionTask::OnGetLatestEpochParameters(
    base::expected<cardano_rpc::EpochParameters, std::string>
        epoch_parameters) {
  if (!epoch_parameters.has_value()) {
    SetError(std::move(epoch_parameters.error()));
    WorkOnTask();
    return;
  }

  latest_epoch_parameters_ = std::move(epoch_parameters.value());
  WorkOnTask();
}

void CreateCardanoTransactionTask::OnGetUtxos(
    base::expected<UtxoMap, std::string> utxos) {
  if (!utxos.has_value()) {
    SetError(std::move(utxos.error()));
    WorkOnTask();
    return;
  }

  if (utxos->empty()) {
    SetError(WalletInternalErrorMessage());
    WorkOnTask();
    return;
  }

  utxo_map_ = std::move(utxos.value());
  WorkOnTask();
}

}  // namespace brave_wallet
