// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_complete_transaction_task.h"

#include <array>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
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
std::unique_ptr<OrchardBundleManager> ApplyOrchardSignatures(
    std::unique_ptr<OrchardBundleManager> orchard_bundle_manager,
    std::array<uint8_t, kZCashDigestSize> sighash) {
  DVLOG(1) << "Apply signatures for ZCash transaction";
  LOG(ERROR)
      << "XXXZZZ ApplyOrchardSignatures - Applying signatures with sighash: "
      << ToHex(sighash);
  // Heavy CPU operation, should be executed on background thread
  auto result = orchard_bundle_manager->ApplySignature(sighash);
  if (result) {
    LOG(ERROR)
        << "XXXZZZ ApplyOrchardSignatures - Signatures applied successfully";
  } else {
    LOG(ERROR)
        << "XXXZZZ ApplyOrchardSignatures - Signature application failed!";
  }
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
    const ZCashTransaction& transaction)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)),
      keyring_service_(keyring_service),
      transaction_(transaction) {}

ZCashCompleteTransactionTask::~ZCashCompleteTransactionTask() = default;

void ZCashCompleteTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashCompleteTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::WorkOnTask() {
  if (error_) {
    LOG(ERROR) << "XXXZZZ ZCashCompleteTransactionTask error: "
               << error_.value()
               << ", account: " << context_.account_id->address;
    LOG(ERROR) << "XXXZZZ WorkOnTask - Transaction failed, calling callback "
                  "with error";
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

#if BUILDFLAG(ENABLE_ORCHARD)
  // Process Orchard part if there are Orchard inputs or outputs
  if (!transaction_.orchard_part().inputs.empty() ||
      !transaction_.orchard_part().outputs.empty()) {
    if (!transaction_.orchard_part().anchor_block_height.has_value()) {
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

  LOG(ERROR) << "XXXZZZ WorkOnTask - Transaction completed successfully, "
                "calling callback";
  std::move(callback_).Run(std::move(transaction_));
}

void ZCashCompleteTransactionTask::Start(
    ZCashCompleteTransactionTaskCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
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
    LOG(ERROR)
        << "XXXZZZ ZCashCompleteTransactionTask - Failed to get lightd info: "
        << result.error() << ", account: " << context_.account_id->address;
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  uint32_t consensus_branch_id;
  if (!base::HexStringToUInt(result.value()->consensusBranchId,
                             &consensus_branch_id)) {
    LOG(ERROR) << "XXXZZZ ZCashCompleteTransactionTask - Failed to parse "
                  "consensus branch ID: "
               << result.value()->consensusBranchId
               << ", account: " << context_.account_id->address;
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
    LOG(ERROR)
        << "XXXZZZ ZCashCompleteTransactionTask - Failed to get latest block: "
        << result.error() << ", account: " << context_.account_id->address;
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  chain_tip_height_ = result.value()->height;

  transaction_.set_locktime(result.value()->height);
  transaction_.set_expiry_height(result.value()->height +
                                 kDefaultZCashBlockHeightDelta);

  LOG(ERROR) << "XXXZZZ get latest bock result: ";
  LOG(ERROR) << "XXXZZZ chain tip block height " << result.value()->height;
  LOG(ERROR) << "XXXZZZ chain tip block hash " << ToHex(result.value()->hash);

  ScheduleWorkOnTask();
}

#if BUILDFLAG(ENABLE_ORCHARD)
void ZCashCompleteTransactionTask::CalculateWitness() {
  if (transaction_.orchard_part().inputs.empty()) {
    witness_inputs_ = std::vector<OrchardInput>();
    ScheduleWorkOnTask();
    return;
  }

  context_.sync_state
      ->AsyncCall(&OrchardSyncState::CalculateWitnessForCheckpoint)
      .WithArgs(context_.account_id.Clone(), transaction_.orchard_part().inputs,
                transaction_.orchard_part().anchor_block_height.value())
      .Then(base::BindOnce(
          &ZCashCompleteTransactionTask::OnWitnessCalculateResult,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::OnWitnessCalculateResult(
    base::expected<std::vector<OrchardInput>, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    LOG(ERROR)
        << "XXXZZZ ZCashCompleteTransactionTask - Failed to calculate witness: "
        << result.error().message
        << ", account: " << context_.account_id->address;
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  witness_inputs_ = result.value();
  for (const auto& input : *witness_inputs_) {
    LOG(ERROR) << "XXXZZZ witness position : " << input.witness->position;
    for (const auto& path_elem : input.witness->merkle_path) {
      LOG(ERROR) << "XXXZZZ path element : " << ToHex(path_elem);
    }
  }
  transaction_.orchard_part().inputs = result.value();
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTask::GetTreeState() {
  context_.zcash_rpc->GetTreeState(
      context_.chain_id,
      zcash::mojom::BlockID::New(
          transaction_.orchard_part().anchor_block_height.value(),
          std::vector<uint8_t>({})),
      base::BindOnce(&ZCashCompleteTransactionTask::OnGetTreeState,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTask::OnGetTreeState(
    base::expected<zcash::mojom::TreeStatePtr, std::string> result) {
  if (!result.has_value()) {
    LOG(ERROR)
        << "XXXZZZ ZCashCompleteTransactionTask - Failed to get tree state: "
        << result.error() << ", account: " << context_.account_id->address;
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  anchor_tree_state_ = std::move(result.value());

  LOG(ERROR) << "XXXZZZ anchor tree state:";
  LOG(ERROR) << "XXXZZZ network " << (*anchor_tree_state_)->network;
  LOG(ERROR) << "XXXZZZ hash " << (*anchor_tree_state_)->hash;
  LOG(ERROR) << "XXXZZZ height " << (*anchor_tree_state_)->height;
  LOG(ERROR) << "XXXZZZ orchard tree " << (*anchor_tree_state_)->orchardTree;
  LOG(ERROR) << "XXXZZZ sapling tree " << (*anchor_tree_state_)->saplingTree;
  LOG(ERROR) << "XXXZZZ time " << (*anchor_tree_state_)->time;

  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTask::SignOrchardPart() {
  auto state_tree_bytes = PrefixedHexStringToBytes(
      base::StrCat({"0x", anchor_tree_state_.value()->orchardTree}));
  if (!state_tree_bytes) {
    LOG(ERROR) << "XXXZZZ ZCashCompleteTransactionTask - Failed to parse state "
                  "tree bytes, account: "
               << context_.account_id->address;
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  auto fvk = zcash_wallet_service_->keyring_service_->GetOrchardFullViewKey(
      context_.account_id);
  auto sk = zcash_wallet_service_->keyring_service_->GetOrchardSpendingKey(
      context_.account_id);
  if (!fvk || !sk) {
    LOG(ERROR) << "XXXZZZ ZCashCompleteTransactionTask - Failed to get Orchard "
                  "keys, account: "
               << context_.account_id->address;
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  LOG(ERROR) << "XXXZZZ SignOrchardPart - Got Orchard keys successfully";
  LOG(ERROR) << "XXXZZZ SignOrchardPart - Consensus branch ID: "
             << consensus_branch_id_.value_or(0);
  LOG(ERROR) << "XXXZZZ SignOrchardPart - Chain tip height: "
             << chain_tip_height_.value_or(0);
  LOG(ERROR) << "XXXZZZ SignOrchardPart - Anchor block height: "
             << transaction_.orchard_part().anchor_block_height.value_or(0);
  OrchardSpendsBundle spends_bundle;
  spends_bundle.sk = *sk;
  spends_bundle.fvk = *fvk;
  spends_bundle.inputs = transaction_.orchard_part().inputs;

  LOG(ERROR) << "XXXZZZ SignOrchardPart - Creating Orchard bundle with "
             << transaction_.orchard_part().inputs.size() << " inputs and "
             << transaction_.orchard_part().outputs.size() << " outputs";

  // Debug witness data
  for (size_t i = 0; i < spends_bundle.inputs.size(); ++i) {
    const auto& input = spends_bundle.inputs[i];
    LOG(ERROR) << "XXXZZZ SignOrchardPart - Input " << i
               << ": amount=" << input.note.amount
               << ", has_witness=" << (input.witness ? "yes" : "no");
    if (input.witness) {
      LOG(ERROR) << "XXXZZZ SignOrchardPart - Input " << i
                 << " witness: position=" << input.witness->position
                 << ", merkle_path_size=" << input.witness->merkle_path.size();
    }
  }

  auto orchard_bundle_manager = OrchardBundleManager::Create(
      *state_tree_bytes, spends_bundle, transaction_.orchard_part().outputs);

  if (!orchard_bundle_manager) {
    LOG(ERROR) << "XXXZZZ ZCashCompleteTransactionTask - Failed to create "
                  "Orchard bundle manager, account: "
               << context_.account_id->address;
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  LOG(ERROR)
      << "XXXZZZ SignOrchardPart - OrchardBundleManager::Create succeeded";

  transaction_.orchard_part().digest =
      orchard_bundle_manager->GetOrchardDigest();

  // Calculate Orchard sighash
  auto sighash =
      ZCashSerializer::CalculateSignatureDigest(transaction_, std::nullopt);

  LOG(ERROR) << "XXXZZZ SignOrchardPart - Calculated sighash: "
             << ToHex(sighash);
  LOG(ERROR)
      << "XXXZZZ SignOrchardPart - Transaction header: consensus_branch_id="
      << transaction_.consensus_brach_id()
      << ", locktime=" << transaction_.locktime()
      << ", expiry_height=" << transaction_.expiry_height();

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
    LOG(ERROR) << "XXXZZZ ZCashCompleteTransactionTask - Failed to apply "
                  "Orchard signatures, account: "
               << context_.account_id->address;
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  LOG(ERROR) << "XXXZZZ OnSignOrchardPartComplete - Orchard signatures applied "
                "successfully";
  transaction_.orchard_part().raw_tx = orchard_bundle_manager->GetRawTxBytes();
  LOG(ERROR)
      << "XXXZZZ OnSignOrchardPartComplete - Raw transaction bytes set, size: "
      << transaction_.orchard_part().raw_tx->size();
  ScheduleWorkOnTask();
}

#endif  // BUILDFLAG(ENABLE_ORCHARD)

void ZCashCompleteTransactionTask::SignTransparentPart() {
  LOG(ERROR)
      << "XXXZZZ SignTransparentPart - Starting transparent part signing";
  LOG(ERROR) << "XXXZZZ SignTransparentPart - Transparent inputs: "
             << transaction_.transparent_part().inputs.size()
             << ", Transparent outputs: "
             << transaction_.transparent_part().outputs.size();

  // Sign transparent part
  if (!ZCashSerializer::SignTransparentPart(
          keyring_service_.get(), context_.account_id, transaction_)) {
    LOG(ERROR) << "XXXZZZ ZCashCompleteTransactionTask - Failed to sign "
                  "transparent part, account: "
               << context_.account_id->address;
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  LOG(ERROR)
      << "XXXZZZ SignTransparentPart - Transparent part signed successfully";
  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
