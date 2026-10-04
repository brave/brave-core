// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_complete_transaction_task_v6.h"

#include <array>
#include <utility>
#include <variant>
#include <vector>

#include "base/check.h"
#include "base/notreached.h"
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
ZCashCompleteTransactionTaskV6::ApplyOrchardSignatures(
    std::unique_ptr<OrchardBundleManager> orchard_bundle_manager,
    std::array<uint8_t, kZCashDigestSize> sighash) {
  // Heavy CPU operation, should be executed on background thread
  auto result = orchard_bundle_manager->ApplySignature(sighash);
  return result;
}

ZCashCompleteTransactionTaskV6::ZCashCompleteTransactionTaskV6(
    std::variant<base::PassKey<class ZCashCompleteTransactionTaskV6Test>,
                 base::PassKey<ZCashWalletService>> pass_key,
    ZCashWalletService& zcash_wallet_service,
    ZCashActionContext context,
    KeyringService& keyring_service,
    const ZCashTransaction& transaction)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)),
      keyring_service_(keyring_service),
      transaction_(transaction) {
  CHECK(transaction_.is_v6());
}

ZCashCompleteTransactionTaskV6::~ZCashCompleteTransactionTaskV6() = default;

void ZCashCompleteTransactionTaskV6::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashCompleteTransactionTaskV6::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTaskV6::WorkOnTask() {
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

  // Build an unauthorized bundle (+digest) for every non-empty pool, then sign
  // them together — the ZIP-246 sighash covers both pool digests.
  for (OrchardPool pool : {OrchardPool::kOrchard, OrchardPool::kIronwood}) {
    auto& shielded = PoolRefV6(pool);
    if (shielded.inputs.empty() && shielded.outputs.empty()) {
      continue;
    }
    if (!shielded.anchor_block_height.has_value()) {
      error_ = "Anchor not selected";
      ScheduleWorkOnTask();
      return;
    }
    auto& state = v6_signing_state_[pool];
    if (!state.witness_done) {
      CalculateWitnessV6(pool);
      return;
    }
    if (!state.tree_state) {
      GetTreeStateV6(pool);
      return;
    }
    if (!shielded.digest) {
      BuildOrchardBundleV6(pool);
      return;
    }
  }
  // Every non-empty pool now has a digest; sign any bundle not yet signed.
  for (OrchardPool pool : {OrchardPool::kOrchard, OrchardPool::kIronwood}) {
    if (v6_signing_state_.count(pool) &&
        v6_signing_state_[pool].bundle_manager && !PoolRefV6(pool).raw_tx) {
      SignOrchardBundlesV6();
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

void ZCashCompleteTransactionTaskV6::Start(
    ZCashCompleteTransactionTaskV6Callback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTaskV6::GetLightdInfo() {
  context_.zcash_rpc->GetLightdInfo(
      context_.chain_id,
      base::BindOnce(&ZCashCompleteTransactionTaskV6::OnGetLightdInfo,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTaskV6::OnGetLightdInfo(
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

void ZCashCompleteTransactionTaskV6::GetLatestBlock() {
  context_.zcash_rpc->GetLatestBlock(
      context_.chain_id,
      base::BindOnce(&ZCashCompleteTransactionTaskV6::OnGetLatestBlockHeight,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCompleteTransactionTaskV6::OnGetLatestBlockHeight(
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

ZCashCompleteTransactionTaskV6::V6PoolSigningState::V6PoolSigningState() =
    default;
ZCashCompleteTransactionTaskV6::V6PoolSigningState::~V6PoolSigningState() =
    default;
ZCashCompleteTransactionTaskV6::V6PoolSigningState::V6PoolSigningState(
    V6PoolSigningState&&) = default;
ZCashCompleteTransactionTaskV6::V6PoolSigningState&
ZCashCompleteTransactionTaskV6::V6PoolSigningState::operator=(
    V6PoolSigningState&&) = default;

ZCashTransaction::ShieldedPool& ZCashCompleteTransactionTaskV6::PoolRefV6(
    OrchardPool pool) {
  CHECK(transaction_.is_v6());
  return pool == OrchardPool::kIronwood ? transaction_.v6_part().ironwood
                                        : transaction_.v6_part().legacy_orchard;
}

void ZCashCompleteTransactionTaskV6::CalculateWitnessV6(OrchardPool pool) {
  auto& shielded = PoolRefV6(pool);
  if (shielded.inputs.empty()) {
    v6_signing_state_[pool].witness_done = true;
    ScheduleWorkOnTask();
    return;
  }
  context_.sync_state
      ->AsyncCall(&OrchardSyncState::CalculateWitnessForCheckpoint)
      .WithArgs(pool, context_.account_id.Clone(), shielded.inputs,
                shielded.anchor_block_height.value())
      .Then(base::BindOnce(
          &ZCashCompleteTransactionTaskV6::OnWitnessCalculateResultV6,
          weak_ptr_factory_.GetWeakPtr(), pool));
}

void ZCashCompleteTransactionTaskV6::OnWitnessCalculateResultV6(
    OrchardPool pool,
    base::expected<std::vector<OrchardInput>, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  PoolRefV6(pool).inputs = result.value();
  v6_signing_state_[pool].witness_done = true;
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTaskV6::GetTreeStateV6(OrchardPool pool) {
  context_.zcash_rpc->GetTreeState(
      context_.chain_id,
      zcash::mojom::BlockID::New(PoolRefV6(pool).anchor_block_height.value(),
                                 std::vector<uint8_t>({})),
      base::BindOnce(&ZCashCompleteTransactionTaskV6::OnGetTreeStateV6,
                     weak_ptr_factory_.GetWeakPtr(), pool));
}

void ZCashCompleteTransactionTaskV6::OnGetTreeStateV6(
    OrchardPool pool,
    base::expected<zcash::mojom::TreeStatePtr, std::string> result) {
  if (!result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  v6_signing_state_[pool].tree_state = std::move(result.value());
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTaskV6::BuildOrchardBundleV6(OrchardPool pool) {
  auto& state = v6_signing_state_[pool];
  auto& shielded = PoolRefV6(pool);

  const std::string& tree_hex = (pool == OrchardPool::kIronwood)
                                    ? state.tree_state.value()->ironwoodTree
                                    : state.tree_state.value()->orchardTree;
  auto state_tree_bytes =
      PrefixedHexStringToBytes(base::StrCat({"0x", tree_hex}));
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
  spends_bundle.inputs = shielded.inputs;
  auto orchard_bundle_manager = OrchardBundleManager::Create(
      *state_tree_bytes, spends_bundle, shielded.outputs, pool,
      /*is_v6_transaction=*/true);
  if (!orchard_bundle_manager) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  shielded.digest = orchard_bundle_manager->GetOrchardDigest();
  state.bundle_manager = std::move(orchard_bundle_manager);
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTaskV6::SignOrchardBundlesV6() {
  if (!v6_sighash_) {
    v6_sighash_ = ZCashSerializerUtils::CalculateSignatureDigest(transaction_,
                                                                 std::nullopt);
  }
  for (OrchardPool pool : {OrchardPool::kOrchard, OrchardPool::kIronwood}) {
    if (!v6_signing_state_.count(pool)) {
      continue;
    }
    auto& state = v6_signing_state_[pool];
    if (state.bundle_manager && !PoolRefV6(pool).raw_tx) {
      auto manager = std::move(state.bundle_manager);
      base::ThreadPool::PostTaskAndReplyWithResult(
          FROM_HERE, {base::MayBlock()},
          base::BindOnce(&ApplyOrchardSignatures, std::move(manager),
                         *v6_sighash_),
          base::BindOnce(
              &ZCashCompleteTransactionTaskV6::OnSignOrchardBundleCompleteV6,
              weak_ptr_factory_.GetWeakPtr(), pool));
      return;
    }
  }
  // WorkOnTask only calls this once it has found a bundle to sign, so falling
  // through here would drop `callback_` and hang the task.
  NOTREACHED();
}

void ZCashCompleteTransactionTaskV6::OnSignOrchardBundleCompleteV6(
    OrchardPool pool,
    std::unique_ptr<OrchardBundleManager> orchard_bundle_manager) {
  if (!orchard_bundle_manager) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  PoolRefV6(pool).raw_tx = orchard_bundle_manager->GetRawTxBytes();
  ScheduleWorkOnTask();
}

void ZCashCompleteTransactionTaskV6::SignTransparentPart() {
  if (!ZCashSerializer::SignTransparentPart(
          keyring_service_.get(), context_.account_id, transaction_)) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
