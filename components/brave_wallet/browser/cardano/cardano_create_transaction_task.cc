/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_create_transaction_task.h"

#include <stdint.h>

#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/bind_post_task.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_knapsack_solver.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

namespace {
std::vector<CardanoTransaction::TxInput> TxInputsFromUtxoMap(
    const std::map<CardanoAddress, cardano_rpc::UnspentOutputs>& map) {
  std::vector<CardanoTransaction::TxInput> result;
  for (const auto& [key, value] : map) {
    for (const auto& utxo : value) {
      if (auto input = CardanoTransaction::TxInput::FromRpcUtxo(key, utxo)) {
        result.push_back(std::move(*input));
      }
    }
  }

  return result;
}

}  // namespace

CardanoCreateTransactionTask::CardanoCreateTransactionTask(
    CardanoWalletService& cardano_wallet_service,
    const mojom::AccountIdPtr& account_id,
    const CardanoAddress& address_to,
    uint64_t amount,
    bool sending_max_amount)
    : cardano_wallet_service_(cardano_wallet_service),
      account_id_(account_id.Clone()),
      address_to_(address_to),
      sending_max_amount_(sending_max_amount) {
  transaction_.set_to(address_to);
  transaction_.set_amount(amount);
  transaction_.set_sending_max_amount(sending_max_amount);
}

CardanoCreateTransactionTask::~CardanoCreateTransactionTask() = default;

void CardanoCreateTransactionTask::Start(Callback callback) {
  callback_ = base::BindPostTaskToCurrentDefault(std::move(callback));
  ScheduleWorkOnTask();
}

void CardanoCreateTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CardanoCreateTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

CardanoTransaction::TxOutput
CardanoCreateTransactionTask::CreateTargetOutput() {
  CardanoTransaction::TxOutput target_output;
  target_output.type = CardanoTransaction::TxOutputType::kTarget;
  target_output.amount =
      transaction_.sending_max_amount() ? 0 : transaction_.amount();
  target_output.address = transaction_.to();

  return target_output;
}

CardanoTransaction::TxOutput
CardanoCreateTransactionTask::CreateChangeOutput() {
  CardanoTransaction::TxOutput change_output;
  change_output.type = CardanoTransaction::TxOutputType::kChange;
  change_output.amount = 0;
  change_output.address =
      *CardanoAddress::FromString(change_address_->address_string);

  return change_output;
}

void CardanoCreateTransactionTask::WorkOnTask() {
  if (!latest_epoch_parameters_) {
    cardano_wallet_service_->cardano_rpc().GetLatestEpochParameters(
        GetNetworkForCardanoAccount(account_id_),
        base::BindOnce(
            &CardanoCreateTransactionTask::OnGetLatestEpochParameters,
            weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (!latest_block_) {
    cardano_wallet_service_->cardano_rpc().GetLatestBlock(
        GetNetworkForCardanoAccount(account_id_),
        base::BindOnce(&CardanoCreateTransactionTask::OnGetLatestBlock,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (!utxo_map_) {
    cardano_wallet_service_->GetUtxos(
        account_id_.Clone(),
        base::BindOnce(&CardanoCreateTransactionTask::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (!change_address_) {
    cardano_wallet_service_->DiscoverNextUnusedAddress(
        account_id_.Clone(), mojom::CardanoKeyRole::kInternal,
        base::BindOnce(
            &CardanoCreateTransactionTask::OnDiscoverNextUnusedChangeAddress,
            weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (!has_solved_transaction_) {
    base::expected<CardanoTransaction, std::string> solved_transaction;

    if (!sending_max_amount_) {
      transaction_.AddOutput(CreateTargetOutput());
      transaction_.AddOutput(CreateChangeOutput());

      CardanoKnapsackSolver solver(transaction_, *latest_epoch_parameters_,
                                   TxInputsFromUtxoMap(*utxo_map_));
      solved_transaction = solver.Solve();
    }

    if (!solved_transaction.has_value()) {
      StopWithError(solved_transaction.error());
      return;
    }

    has_solved_transaction_ = true;
    transaction_ = std::move(*solved_transaction);
  }

  std::move(callback_).Run(base::ok(std::move(transaction_)));
}

void CardanoCreateTransactionTask::StopWithError(std::string error_string) {
  std::move(callback_).Run(base::unexpected(std::move(error_string)));
}

void CardanoCreateTransactionTask::OnGetLatestEpochParameters(
    base::expected<cardano_rpc::EpochParameters, std::string>
        epoch_parameters) {
  if (!epoch_parameters.has_value()) {
    StopWithError(std::move(epoch_parameters.error()));
    return;
  }

  latest_epoch_parameters_ = std::move(epoch_parameters.value());
  WorkOnTask();
}

void CardanoCreateTransactionTask::OnGetLatestBlock(
    base::expected<cardano_rpc::Block, std::string> block) {
  if (!block.has_value()) {
    StopWithError(std::move(block.error()));
    return;
  }

  latest_block_ = std::move(block.value());
  WorkOnTask();
}

void CardanoCreateTransactionTask::OnGetUtxos(
    base::expected<UtxoMap, std::string> utxos) {
  if (!utxos.has_value()) {
    StopWithError(std::move(utxos.error()));
    return;
  }

  if (utxos->empty()) {
    StopWithError(WalletInternalErrorMessage());
    return;
  }

  utxo_map_ = std::move(utxos.value());
  WorkOnTask();
}

void CardanoCreateTransactionTask::OnDiscoverNextUnusedChangeAddress(
    base::expected<mojom::CardanoAddressPtr, std::string> address) {
  if (!address.has_value()) {
    StopWithError(std::move(address.error()));
    return;
  }
  DCHECK_EQ(address.value()->payment_key_id->role,
            mojom::CardanoKeyRole::kInternal);
  // TODO(https://github.com/brave/brave-browser/issues/45278): should update
  // account pref with new address.
  change_address_ = std::move(address.value());
  WorkOnTask();
}

}  // namespace brave_wallet
