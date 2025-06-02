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
#include "brave/components/brave_wallet/browser/cardano/cardano_max_send_solver.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

// Transaction is valid for 2 hours.
// https://github.com/input-output-hk/cardano-js-sdk/blob/5bc90ee9f24d89db6ea4191d705e7383d52fef6a/packages/tx-construction/src/ensureValidityInterval.ts#L3
constexpr uint32_t kTxValiditySeconds = 2 * 3600;

namespace {
static bool g_arrange_tx_for_test = false;

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
  CHECK(IsCardanoAccount(account_id));

  transaction_.set_to(address_to);
  transaction_.set_amount(amount);
  transaction_.set_sending_max_amount(sending_max_amount);
}

CardanoCreateTransactionTask::~CardanoCreateTransactionTask() = default;

// static
void CardanoCreateTransactionTask::SetArrangeTransactionForTesting(bool value) {
  g_arrange_tx_for_test = value;
}

void CardanoCreateTransactionTask::Start(Callback callback) {
  callback_ = base::BindPostTaskToCurrentDefault(std::move(callback));
  FetchAllRequiredData();
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

cardano_rpc::CardanoRpc* CardanoCreateTransactionTask::GetCardanoRpc() {
  return cardano_wallet_service_->GetCardanoRpc(
      GetNetworkForCardanoAccount(account_id_));
}

void CardanoCreateTransactionTask::FetchAllRequiredData() {
  GetCardanoRpc()->GetLatestEpochParameters(
      base::BindOnce(&CardanoCreateTransactionTask::OnGetLatestEpochParameters,
                     weak_ptr_factory_.GetWeakPtr()));
  GetCardanoRpc()->GetLatestBlock(
      base::BindOnce(&CardanoCreateTransactionTask::OnGetLatestBlock,
                     weak_ptr_factory_.GetWeakPtr()));
  cardano_wallet_service_->GetUtxos(
      account_id_.Clone(),
      base::BindOnce(&CardanoCreateTransactionTask::OnGetUtxos,
                     weak_ptr_factory_.GetWeakPtr()));
  cardano_wallet_service_->DiscoverNextUnusedAddress(
      account_id_.Clone(), mojom::CardanoKeyRole::kExternal,
      base::BindOnce(
          &CardanoCreateTransactionTask::OnDiscoverNextUnusedChangeAddress,
          weak_ptr_factory_.GetWeakPtr()));
}

bool CardanoCreateTransactionTask::IsAllRequiredDataFetched() {
  return latest_epoch_parameters_ && latest_block_ && utxo_map_ &&
         change_address_;
}

void CardanoCreateTransactionTask::OnMaybeAllRequiredDataFetched() {
  if (IsAllRequiredDataFetched()) {
    RunSolverForTransaction();
  }
}

void CardanoCreateTransactionTask::RunSolverForTransaction() {
  CHECK(IsAllRequiredDataFetched());

  base::expected<CardanoTransaction, std::string> solved_transaction;
  transaction_.set_invalid_after(latest_block_->slot + kTxValiditySeconds);

  if (sending_max_amount_) {
    transaction_.AddOutput(CreateTargetOutput());

    CardanoMaxSendSolver solver(transaction_, *latest_epoch_parameters_,
                                TxInputsFromUtxoMap(*utxo_map_));
    solved_transaction = solver.Solve();
  } else {
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

  transaction_ = std::move(*solved_transaction);
  if (g_arrange_tx_for_test) {
    transaction_.ArrangeTransactionForTesting();  // IN-TEST
  }

  StopWithResult(std::move(transaction_));
}

void CardanoCreateTransactionTask::StopWithError(std::string error_string) {
  weak_ptr_factory_.InvalidateWeakPtrs();

  std::move(callback_).Run(base::unexpected(std::move(error_string)));
}

void CardanoCreateTransactionTask::StopWithResult(CardanoTransaction result) {
  weak_ptr_factory_.InvalidateWeakPtrs();

  std::move(callback_).Run(base::ok(std::move(result)));
}

void CardanoCreateTransactionTask::OnGetLatestEpochParameters(
    base::expected<cardano_rpc::EpochParameters, std::string>
        epoch_parameters) {
  if (!epoch_parameters.has_value()) {
    StopWithError(std::move(epoch_parameters.error()));
    return;
  }

  latest_epoch_parameters_ = std::move(epoch_parameters.value());
  OnMaybeAllRequiredDataFetched();
}

void CardanoCreateTransactionTask::OnGetLatestBlock(
    base::expected<cardano_rpc::Block, std::string> block) {
  if (!block.has_value()) {
    StopWithError(std::move(block.error()));
    return;
  }

  latest_block_ = std::move(block.value());
  OnMaybeAllRequiredDataFetched();
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
  OnMaybeAllRequiredDataFetched();
}

void CardanoCreateTransactionTask::OnDiscoverNextUnusedChangeAddress(
    base::expected<mojom::CardanoAddressPtr, std::string> address) {
  if (!address.has_value()) {
    StopWithError(std::move(address.error()));
    return;
  }
  // TODO(https://github.com/brave/brave-browser/issues/45278): we support only
  // simple Cardano accounts now when there is only one address per account. So
  // change address is also external address.
  DCHECK_EQ(address.value()->payment_key_id->role,
            mojom::CardanoKeyRole::kExternal);
  // TODO(https://github.com/brave/brave-browser/issues/45278): should update
  // account pref with new address.
  change_address_ = std::move(address.value());
  OnMaybeAllRequiredDataFetched();
}

}  // namespace brave_wallet
