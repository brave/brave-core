// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/create_shield_all_transaction_task.h"

#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

std::unique_ptr<OrchardBundleManager> ApplyOrchardSignatures(
    std::unique_ptr<OrchardBundleManager> orchard_bundle_manager,
    std::array<uint8_t, kZCashDigestSize> sighash) {
  // Heavy CPU operation, should be executed on background thread
  return orchard_bundle_manager->ApplySignature(sighash);
}

// Creates address key id for receiving funds on internal orchard address
mojom::ZCashKeyIdPtr CreateOrchardInternalKeyId(
    const mojom::AccountIdPtr& accoint_id) {
  return mojom::ZCashKeyId::New(accoint_id->account_index, 1 /* internal */, 0);
}

}  // namespace

CreateShieldAllTransactionTask::CreateShieldAllTransactionTask(
    ZCashWalletService* zcash_wallet_service,
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    ZCashWalletService::CreateTransactionCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      chain_id_(chain_id),
      account_id_(account_id.Clone()),
      callback_(std::move(callback)) {}

CreateShieldAllTransactionTask::~CreateShieldAllTransactionTask() = default;

void CreateShieldAllTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CreateShieldAllTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CreateShieldAllTransactionTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    return;
  }

  if (!tree_state_) {
    GetTreeState();
    return;
  }

  if (!utxo_map_) {
    GetAllUtxos();
    return;
  }

  if (!chain_height_) {
    GetChainHeight();
    return;
  }

  if (!transaction_ && !CreateTransaction()) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    return;
  }

  // Signing of transparent part is final step
  if (!transaction_->IsTransparentPartSigned()) {
    CompleteTransaction();
    return;
  }

  std::move(callback_).Run(std::move(transaction_.value()));
}

bool CreateShieldAllTransactionTask::CreateTransaction() {
  CHECK(utxo_map_);
  CHECK(chain_height_);

  ZCashTransaction zcash_transaction;

  // Pick inputs
  std::vector<ZCashTransaction::TxInput> all_inputs;
  for (const auto& item : utxo_map_.value()) {
    for (const auto& utxo : item.second) {
      if (!utxo) {
        error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
        return false;
      }
      if (auto input =
              ZCashTransaction::TxInput::FromRpcUtxo(item.first, *utxo)) {
        all_inputs.emplace_back(std::move(*input));
      }
    }
  }

  zcash_transaction.transparent_part().inputs = std::move(all_inputs);
  // TODO(cypt4): Calculate orchard actions count
  zcash_transaction.set_fee(CalculateZCashTxFee(
      zcash_transaction.transparent_part().inputs.size(),
      2 /* actions count for 1 orchard output no orchard inputs */));

  // Pick orchard outputs
  ZCashTransaction::OrchardOutput orchard_output;
  auto addr_bytes =
      zcash_wallet_service_->keyring_service()->GetOrchardRawBytes(
          account_id_, CreateOrchardInternalKeyId(account_id_));
  if (!addr_bytes) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return false;
  }

  if (zcash_transaction.fee() > zcash_transaction.TotalInputsAmount()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return false;
  }

  orchard_output.value =
      zcash_transaction.TotalInputsAmount() - zcash_transaction.fee();
  orchard_output.address = std::move(addr_bytes.value());

  zcash_transaction.orchard_part().outputs.push_back(std::move(orchard_output));

  zcash_transaction.set_locktime(chain_height_.value());
  zcash_transaction.set_expiry_height(chain_height_.value() +
                                      kDefaultZCashBlockHeightDelta);

  transaction_ = std::move(zcash_transaction);

  return true;
}

void CreateShieldAllTransactionTask::CompleteTransaction() {
  CHECK(transaction_);
  CHECK_EQ(1u, transaction_->orchard_part().outputs.size());
  CHECK(tree_state_);

  // Sign shielded part
  auto state_tree_bytes = PrefixedHexStringToBytes(
      base::StrCat({"0x", tree_state_.value()->orchardTree}));
  if (!state_tree_bytes) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  std::vector<OrchardOutput> outputs;
  for (const auto& output : transaction_->orchard_part().outputs) {
    outputs.push_back(OrchardOutput{output.value, output.address});
  }

  auto orchard_bundle_manager =
      OrchardBundleManager::Create(*state_tree_bytes, std::move(outputs));

  if (!orchard_bundle_manager) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  transaction_->orchard_part().digest =
      orchard_bundle_manager->GetOrchardDigest();

  // Calculate Orchard sighash
  auto sighash = ZCashSerializer::CalculateSignatureDigest(transaction_.value(),
                                                           std::nullopt);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ApplyOrchardSignatures, std::move(orchard_bundle_manager),
                     sighash),
      base::BindOnce(&CreateShieldAllTransactionTask::OnSignatureApplied,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CreateShieldAllTransactionTask::OnSignatureApplied(
    std::unique_ptr<OrchardBundleManager> orchard_bundle_manager) {
  if (!orchard_bundle_manager) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  transaction_->orchard_part().raw_tx = orchard_bundle_manager->GetRawTxBytes();

  // Sign transparent part
  if (!zcash_wallet_service_->SignTransactionInternal(transaction_.value(),
                                                      account_id_)) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  ScheduleWorkOnTask();
}

void CreateShieldAllTransactionTask::GetAllUtxos() {
  zcash_wallet_service_->GetUtxos(
      chain_id_, account_id_.Clone(),
      base::BindOnce(&CreateShieldAllTransactionTask::OnGetUtxos,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CreateShieldAllTransactionTask::GetTreeState() {
  zcash_wallet_service_->zcash_rpc()->GetLatestTreeState(
      chain_id_, base::BindOnce(&CreateShieldAllTransactionTask::OnGetTreeState,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CreateShieldAllTransactionTask::GetChainHeight() {
  zcash_wallet_service_->zcash_rpc()->GetLatestBlock(
      chain_id_,
      base::BindOnce(&CreateShieldAllTransactionTask::OnGetChainHeight,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CreateShieldAllTransactionTask::OnGetUtxos(
    base::expected<ZCashWalletService::UtxoMap, std::string> utxo_map) {
  if (!utxo_map.has_value()) {
    error_ = utxo_map.error();
  } else {
    utxo_map_ = std::move(*utxo_map);
  }

  WorkOnTask();
}

void CreateShieldAllTransactionTask::OnGetTreeState(
    base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state) {
  if (!tree_state.has_value()) {
    error_ = tree_state.error();
  } else {
    tree_state_ = std::move(*tree_state);
  }

  WorkOnTask();
}

void CreateShieldAllTransactionTask::OnGetChainHeight(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    error_ = result.error();
  } else {
    chain_height_ = (*result)->height;
  }

  WorkOnTask();
}

}  // namespace brave_wallet
