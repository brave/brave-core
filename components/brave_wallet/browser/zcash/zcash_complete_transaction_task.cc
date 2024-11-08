// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_complete_transaction_task.h"

#include <utility>
#include <vector>

#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

constexpr size_t kMinConfirmations = 10;

#if BUILDFLAG(ENABLE_ORCHARD)
std::unique_ptr<OrchardBundleManager> ApplyOrchardSignatures(
    std::unique_ptr<OrchardBundleManager> orchard_bundle_manager,
    std::array<uint8_t, kZCashDigestSize> sighash) {
  DVLOG(1) << "Apply signatures for ZCash transaction";
  // Heavy CPU operation, should be executed on background thread
  auto result = orchard_bundle_manager->ApplySignature(sighash);
  DVLOG(1) << "Signatures applied";
  return result;
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace

ZCashCompleteTransactionTask::ZCashCompleteTransactionTask(
    ZCashWalletService* zcash_wallet_service,
    ZCashRpc& zcash_rpc,
    base::SequenceBound<OrchardSyncState>& sync_state,
    KeyringService& keyring_service,
    const mojom::AccountIdPtr& account_id,
    const std::string& chain_id,
    const ZCashTransaction& transaction,
    ZCashCompleteTransactionTaskCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      zcash_rpc_(zcash_rpc),
      sync_state_(sync_state),
      keyring_service_(keyring_service),
      account_id_(account_id.Clone()),
      chain_id_(chain_id),
      transaction_(transaction),
      callback_(std::move(callback)) {}

void ZCashCompleteTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashCompleteTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    zcash_wallet_service_->CompleteTransactionTaskDone(this);
    return;
  }

  if (!chain_tip_height_) {
    GetLatestBlock();
    return;
  }

#if BUILDFLAG(ENABLE_ORCHARD)
  if (!transaction_.orchard_part().outputs.empty()) {
    if (!anchor_block_height_) {
      GetMaxCheckpointedHeight();
      return;
    }

    if (!witness_inputs_) {
      CalculateWitness();
      return;
    }

    if (!anchor_tree_state_) {
      GetTreeState();
      return;
    }

    if (!transaction_.orchard_part().raw_tx) {
      SignOrchardPart();
      return;
    }
  }
#endif  // BUILDFLAG(ENABLE_ORCHARD)

  if (!transaction_.transparent_part().inputs.empty() &&
      !transaction_.IsTransparentPartSigned()) {
    SignTransparentPart();
    return;
  }

  std::move(callback_).Run(std::move(transaction_));
  zcash_wallet_service_->CompleteTransactionTaskDone(this);
}

void ZCashCompleteTransactionTask::Start() {
  ScheduleWorkOnTask();
}

ZCashCompleteTransactionTask::~ZCashCompleteTransactionTask() {}

void ZCashCompleteTransactionTask::GetLatestBlock() {
  zcash_rpc_->GetLatestBlock(
      chain_id_,
      base::BindOnce(&ZCashCompleteTransactionTask::OnGetLatestBlockHeight,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::OnGetLatestBlockHeight(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    std::move(callback_).Run(base::unexpected("block height error"));
    ScheduleWorkOnTask();
    return;
  }

  chain_tip_height_ = result.value()->height;

  transaction_.set_locktime(result.value()->height);
  transaction_.set_expiry_height(result.value()->height +
                                 kDefaultZCashBlockHeightDelta);
  ScheduleWorkOnTask();
}

#if BUILDFLAG(ENABLE_ORCHARD)

void ZCashCompleteTransactionTask::GetMaxCheckpointedHeight() {
  CHECK(chain_tip_height_);
  if (transaction_.orchard_part().inputs.empty()) {
    anchor_block_height_ = chain_tip_height_;
    ScheduleWorkOnTask();
    return;
  }
  sync_state_->AsyncCall(&OrchardSyncState::GetMaxCheckpointedHeight)
      .WithArgs(account_id_.Clone(), chain_tip_height_.value(),
                kMinConfirmations)
      .Then(base::BindOnce(
          &ZCashCompleteTransactionTask::OnGetMaxCheckpointedHeight,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::OnGetMaxCheckpointedHeight(
    base::expected<std::optional<uint32_t>, OrchardStorage::Error> result) {
  if (!result.has_value() || !result.value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  anchor_block_height_ = *result;
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTask::CalculateWitness() {
  if (transaction_.orchard_part().inputs.empty()) {
    witness_inputs_ = std::vector<OrchardInput>();
    ScheduleWorkOnTask();
    return;
  }

  sync_state_->AsyncCall(&OrchardSyncState::CalculateWitnessForCheckpoint)
      .WithArgs(account_id_.Clone(), transaction_.orchard_part().inputs,
                anchor_block_height_.value())
      .Then(base::BindOnce(
          &ZCashCompleteTransactionTask::OnWitnessCalulcateResult,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::OnWitnessCalulcateResult(
    base::expected<std::vector<OrchardInput>, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  // TODO(cypt4): rewrite
  witness_inputs_ = result.value();
  transaction_.orchard_part().inputs = result.value();
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTask::GetTreeState() {
  zcash_rpc_->GetTreeState(
      chain_id_,
      zcash::mojom::BlockID::New(anchor_block_height_.value(),
                                 std::vector<uint8_t>({})),
      base::BindOnce(&ZCashCompleteTransactionTask::OnGetTreeState,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::OnGetTreeState(
    base::expected<zcash::mojom::TreeStatePtr, std::string> result) {
  if (!result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  anchor_tree_state_ = std::move(result.value());
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTask::SignOrchardPart() {
  // Sign shielded part
  auto state_tree_bytes = PrefixedHexStringToBytes(
      base::StrCat({"0x", anchor_tree_state_.value()->orchardTree}));
  if (!state_tree_bytes) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  auto fvk = zcash_wallet_service_->keyring_service_->GetOrchardFullViewKey(
      account_id_);
  auto sk = zcash_wallet_service_->keyring_service_->GetOrchardSpendingKey(
      account_id_);
  if (!fvk || !sk) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  OrchardSpendsBundle spends_bundle;
  spends_bundle.sk = *sk;
  spends_bundle.fvk = *fvk;
  spends_bundle.inputs = transaction_.orchard_part().inputs;
  auto orchard_bundle_manager = OrchardBundleManager::Create(
      *state_tree_bytes, spends_bundle, transaction_.orchard_part().outputs);

  if (!orchard_bundle_manager) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  transaction_.orchard_part().digest =
      orchard_bundle_manager->GetOrchardDigest();

  // Calculate Orchard sighash
  auto sighash =
      ZCashSerializer::CalculateSignatureDigest(transaction_, std::nullopt);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ApplyOrchardSignatures, std::move(orchard_bundle_manager),
                     sighash),
      base::BindOnce(&ZCashCompleteTransactionTask::OnSignOrchardPartComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::OnSignOrchardPartComplete(
    std::unique_ptr<OrchardBundleManager> orchard_bundle_manager) {
  if (!orchard_bundle_manager) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  transaction_.orchard_part().raw_tx = orchard_bundle_manager->GetRawTxBytes();
  ScheduleWorkOnTask();
}

#endif  // BUILDFLAG(ENABLE_ORCHARD)

void ZCashCompleteTransactionTask::SignTransparentPart() {
  // Sign transparent part
  if (!ZCashSerializer::SignTransparentPart(keyring_service_.get(), account_id_,
                                            transaction_)) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
