// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_complete_manager.h"

#include <utility>

#include "base/task/thread_pool.h"
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
  // Heavy CPU operation, should be executed on background thread
  return orchard_bundle_manager->ApplySignature(sighash);
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace

ZCashTransactionCompleteManager::ParamsBundle::ParamsBundle(
    std::string chain_id,
    ZCashTransaction transaction,
    mojom::AccountIdPtr account_id,
    CompleteTransactionCallback callback)
    : chain_id(std::move(chain_id)),
      transaction(std::move(transaction)),
      account_id(std::move(account_id)),
      callback(std::move(callback)) {}
ZCashTransactionCompleteManager::ParamsBundle::~ParamsBundle() {}
ZCashTransactionCompleteManager::ParamsBundle::ParamsBundle(
    ParamsBundle&& other) = default;

ZCashTransactionCompleteManager::ZCashTransactionCompleteManager(
    ZCashWalletService* zcash_wallet_service)
    : zcash_wallet_service_(zcash_wallet_service) {}

ZCashTransactionCompleteManager::~ZCashTransactionCompleteManager() {}

void ZCashTransactionCompleteManager::CompleteTransaction(
    const std::string& chain_id,
    const ZCashTransaction& transaction,
    const mojom::AccountIdPtr& account_id,
    CompleteTransactionCallback callback) {
  zcash_wallet_service_->zcash_rpc()->GetLightdInfo(
      chain_id,
      base::BindOnce(&ZCashTransactionCompleteManager::OnGetLightdInfo,
                     weak_ptr_factory_.GetWeakPtr(),
                     ParamsBundle{chain_id, transaction, account_id.Clone(),
                                  std::move(callback)}));
}

void ZCashTransactionCompleteManager::OnGetLatestBlockHeight(
    ParamsBundle params,
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    std::move(params.callback).Run(base::unexpected("block height error"));
    return;
  }

  params.transaction.set_locktime(result.value()->height);
  params.transaction.set_expiry_height(result.value()->height +
                                       kDefaultZCashBlockHeightDelta);

#if BUILDFLAG(ENABLE_ORCHARD)
  if (params.transaction.orchard_part().outputs.empty()) {
    SignTransparentPart(std::move(params));
    return;
  }
  std::string chain_id = params.chain_id;
  zcash_wallet_service_->zcash_rpc()->GetLatestTreeState(
      chain_id,
      base::BindOnce(&ZCashTransactionCompleteManager::OnGetTreeState,
                     weak_ptr_factory_.GetWeakPtr(), std::move(params)));
#else
  SignTransparentPart(std::move(params));
#endif  // BUILDFLAG(ENABLE_ORCHARD)
}

#if BUILDFLAG(ENABLE_ORCHARD)

void ZCashTransactionCompleteManager::OnGetTreeState(
    ParamsBundle params,
    base::expected<zcash::mojom::TreeStatePtr, std::string> result) {
  if (!result.has_value()) {
    std::move(params.callback)
        .Run(base::unexpected(
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  // Sign shielded part
  auto state_tree_bytes = PrefixedHexStringToBytes(
      base::StrCat({"0x", result.value()->orchardTree}));
  if (!state_tree_bytes) {
    std::move(params.callback)
        .Run(base::unexpected(
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  CHECK_EQ(params.transaction.orchard_part().outputs.size(), 1u);
  auto orchard_bundle_manager = OrchardBundleManager::Create(
      *state_tree_bytes, params.transaction.orchard_part().outputs);

  if (!orchard_bundle_manager) {
    std::move(params.callback)
        .Run(base::unexpected(
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  params.transaction.orchard_part().digest =
      orchard_bundle_manager->GetOrchardDigest();

  // Calculate Orchard sighash
  auto sighash = ZCashSerializer::CalculateSignatureDigest(params.transaction,
                                                           std::nullopt);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ApplyOrchardSignatures, std::move(orchard_bundle_manager),
                     sighash),
      base::BindOnce(
          &ZCashTransactionCompleteManager::OnSignOrchardPartComplete,
          weak_ptr_factory_.GetWeakPtr(), std::move(params)));
}

void ZCashTransactionCompleteManager::OnSignOrchardPartComplete(
    ParamsBundle params,
    std::unique_ptr<OrchardBundleManager> orchard_bundle_manager) {
  if (!orchard_bundle_manager) {
    std::move(params.callback)
        .Run(base::unexpected(
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }
  params.transaction.orchard_part().raw_tx =
      orchard_bundle_manager->GetRawTxBytes();

  SignTransparentPart(std::move(params));
}

#endif  // BUILDFLAG(ENABLE_ORCHARD)

void ZCashTransactionCompleteManager::OnGetLightdInfo(
    ParamsBundle params,
    base::expected<zcash::mojom::LightdInfoPtr, std::string> result) {
  if (!result.has_value()) {
    std::move(params.callback).Run(base::unexpected("get lightd info error"));
    return;
  }

  uint32_t consensus_branch_id;
  if (!base::HexStringToUInt(result.value()->consensusBranchId,
                             &consensus_branch_id)) {
    std::move(params.callback)
        .Run(base::unexpected("wrong consensus branch format"));
    return;
  }

  params.transaction.set_consensus_brach_id(consensus_branch_id);
  std::string chain_id = params.chain_id;

  zcash_wallet_service_->zcash_rpc().GetLatestBlock(
      chain_id,
      base::BindOnce(&ZCashTransactionCompleteManager::OnGetLatestBlockHeight,
                     weak_ptr_factory_.GetWeakPtr(), std::move(params)));
}

void ZCashTransactionCompleteManager::SignTransparentPart(ParamsBundle params) {
  // Sign transparent part
  if (!ZCashSerializer::SignTransparentPart(
          zcash_wallet_service_->keyring_service(), params.account_id,
          params.transaction)) {
    std::move(params.callback)
        .Run(base::unexpected(
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  std::move(params.callback).Run(std::move(params.transaction));
}

}  // namespace brave_wallet
