// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_complete_transaction_task_v5.h"

#include <array>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_serializer_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

// static
std::unique_ptr<OrchardBundleManager>
ZCashCompleteTransactionTaskV5::ApplyOrchardSignatures(
    std::unique_ptr<OrchardBundleManager> orchard_bundle_manager,
    std::array<uint8_t, kZCashDigestSize> sighash) {
  // Heavy CPU operation, should be executed on background thread
  auto result = orchard_bundle_manager->ApplySignature(sighash);
  return result;
}

ZCashCompleteTransactionTaskV5::ZCashCompleteTransactionTaskV5(
    base::PassKey<ZCashWalletService> pass_key,
    ZCashWalletService& zcash_wallet_service,
    ZCashActionContext context,
    KeyringService& keyring_service,
    const ZCashTransaction& transaction)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)),
      keyring_service_(keyring_service),
      transaction_(transaction) {
  CHECK(transaction_.is_v5());
}

ZCashCompleteTransactionTaskV5::~ZCashCompleteTransactionTaskV5() = default;

void ZCashCompleteTransactionTaskV5::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashCompleteTransactionTaskV5::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTaskV5::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
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

  if (!transaction_.v5_part().orchard.inputs.empty() ||
      !transaction_.v5_part().orchard.outputs.empty()) {
    if (!transaction_.v5_part().orchard.anchor_block_height.has_value()) {
      error_ = "Anchor not selected";
      ScheduleWorkOnTask();
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

    if (!transaction_.v5_part().orchard.raw_tx) {
      SignOrchardPart();
      return;
    }
  }

  if (!transaction_.transparent_part().inputs.empty() &&
      !transaction_.IsTransparentPartSigned()) {
    SignTransparentPart();
    return;
  }

  std::move(callback_).Run(std::move(transaction_));
}

void ZCashCompleteTransactionTaskV5::Start(
    ZCashCompleteTransactionTaskV5Callback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTaskV5::GetLightdInfo() {
  context_.zcash_rpc->GetLightdInfo(
      context_.chain_id,
      base::BindOnce(&ZCashCompleteTransactionTaskV5::OnGetLightdInfo,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTaskV5::OnGetLightdInfo(
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

void ZCashCompleteTransactionTaskV5::GetLatestBlock() {
  context_.zcash_rpc->GetLatestBlock(
      context_.chain_id,
      base::BindOnce(&ZCashCompleteTransactionTaskV5::OnGetLatestBlockHeight,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTaskV5::OnGetLatestBlockHeight(
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

void ZCashCompleteTransactionTaskV5::CalculateWitness() {
  if (transaction_.v5_part().orchard.inputs.empty()) {
    witness_inputs_ = std::vector<OrchardInput>();
    ScheduleWorkOnTask();
    return;
  }

  context_.sync_state
      ->AsyncCall(&OrchardSyncState::CalculateWitnessForCheckpoint)
      .WithArgs(OrchardPool::kOrchard, context_.account_id.Clone(),
                transaction_.v5_part().orchard.inputs,
                transaction_.v5_part().orchard.anchor_block_height.value())
      .Then(base::BindOnce(
          &ZCashCompleteTransactionTaskV5::OnWitnessCalculateResult,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTaskV5::OnWitnessCalculateResult(
    base::expected<std::vector<OrchardInput>, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  witness_inputs_ = result.value();
  transaction_.v5_part().orchard.inputs = result.value();
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTaskV5::GetTreeState() {
  context_.zcash_rpc->GetTreeState(
      context_.chain_id,
      zcash::mojom::BlockID::New(
          transaction_.v5_part().orchard.anchor_block_height.value(),
          std::vector<uint8_t>({})),
      base::BindOnce(&ZCashCompleteTransactionTaskV5::OnGetTreeState,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTaskV5::OnGetTreeState(
    base::expected<zcash::mojom::TreeStatePtr, std::string> result) {
  if (!result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  anchor_tree_state_ = std::move(result.value());

  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTaskV5::SignOrchardPart() {
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
  spends_bundle.inputs = transaction_.v5_part().orchard.inputs;
  auto orchard_bundle_manager = OrchardBundleManager::Create(
      *state_tree_bytes, spends_bundle, transaction_.v5_part().orchard.outputs,
      OrchardPool::kOrchard, /*is_v6_transaction=*/false);

  if (!orchard_bundle_manager) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  transaction_.v5_part().orchard.digest =
      orchard_bundle_manager->GetOrchardDigest();

  auto sighash = ZCashSerializerUtils::CalculateSignatureDigest(transaction_,
                                                                std::nullopt);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ApplyOrchardSignatures, std::move(orchard_bundle_manager),
                     sighash),
      base::BindOnce(&ZCashCompleteTransactionTaskV5::OnSignOrchardPartComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTaskV5::OnSignOrchardPartComplete(
    std::unique_ptr<OrchardBundleManager> orchard_bundle_manager) {
  if (!orchard_bundle_manager) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  transaction_.v5_part().orchard.raw_tx =
      orchard_bundle_manager->GetRawTxBytes();
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTaskV5::SignTransparentPart() {
  if (!ZCashSerializer::SignTransparentPart(
          keyring_service_.get(), context_.account_id, transaction_)) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
