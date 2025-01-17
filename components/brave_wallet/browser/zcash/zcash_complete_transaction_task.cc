// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_complete_transaction_task.h"

#include <utility>
#include <vector>

#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

#if BUILDFLAG(ENABLE_ORCHARD)
// https://github.com/zcash/librustzcash/blob/2ec38bae002c4763ecda3ac9371e3e367b383fcc/zcash_client_backend/CHANGELOG.md?plain=1#L1264
constexpr size_t kMinConfirmations = 10;

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
    base::PassKey<ZCashWalletService> pass_key,
    ZCashWalletService& zcash_wallet_service,
    ZCashActionContext context,
    KeyringService& keyring_service,
    const ZCashTransaction& transaction,
    ZCashCompleteTransactionTaskCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)),
      keyring_service_(keyring_service),
      transaction_(transaction),
      callback_(std::move(callback)) {}

ZCashCompleteTransactionTask::~ZCashCompleteTransactionTask() = default;

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

  if (!consensus_branch_id_) {
    GetLightdInfo();
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

void ZCashCompleteTransactionTask::GetLightdInfo() {
  context_.zcash_rpc->GetLightdInfo(
      context_.chain_id,
      base::BindOnce(&ZCashCompleteTransactionTask::OnGetLightdInfo,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::OnGetLightdInfo(
    base::expected<zcash::mojom::LightdInfoPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  uint32_t consensus_branch_id;
  if (!base::HexStringToUInt(result.value()->consensusBranchId,
                             &consensus_branch_id)) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  consensus_branch_id_ = consensus_branch_id;
  transaction_.set_consensus_brach_id(consensus_branch_id);
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTask::GetLatestBlock() {
  context_.zcash_rpc->GetLatestBlock(
      context_.chain_id,
      base::BindOnce(&ZCashCompleteTransactionTask::OnGetLatestBlockHeight,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::OnGetLatestBlockHeight(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
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
  context_.sync_state->AsyncCall(&OrchardSyncState::GetMaxCheckpointedHeight)
      .WithArgs(context_.account_id.Clone(), chain_tip_height_.value(),
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

  context_.sync_state
      ->AsyncCall(&OrchardSyncState::CalculateWitnessForCheckpoint)
      .WithArgs(context_.account_id.Clone(), transaction_.orchard_part().inputs,
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

  witness_inputs_ = result.value();
  transaction_.orchard_part().inputs = result.value();
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTask::GetTreeState() {
  context_.zcash_rpc->GetTreeState(
      context_.chain_id,
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
  auto state_tree_bytes = PrefixedHexStringToBytes(
      base::StrCat({"0x", anchor_tree_state_.value()->orchardTree}));
  if (!state_tree_bytes) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  auto fvk = zcash_wallet_service_->keyring_service_->GetOrchardFullViewKey(
      context_.account_id);
  auto sk = zcash_wallet_service_->keyring_service_->GetOrchardSpendingKey(
      context_.account_id);
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
  if (!ZCashSerializer::SignTransparentPart(
          keyring_service_.get(), context_.account_id, transaction_)) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
