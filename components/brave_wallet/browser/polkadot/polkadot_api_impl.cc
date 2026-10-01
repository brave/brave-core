/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_api_impl.h"

#include <array>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/barrier_closure.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

mojom::PolkadotProviderErrorBundlePtr RejectedError() {
  return mojom::PolkadotProviderErrorBundle::New(
      mojom::PolkadotProviderError::kUnknown,
      l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
}

mojom::PolkadotProviderErrorBundlePtr InternalError() {
  return mojom::PolkadotProviderErrorBundle::New(
      mojom::PolkadotProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

}  // namespace

PolkadotApiImpl::PolkadotApiImpl(
    KeyringService& keyring_service,
    PolkadotWalletService& polkadot_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    mojom::AccountIdPtr granted_account,
    const url::Origin& origin)
    : keyring_service_(keyring_service),
      polkadot_wallet_service_(polkadot_wallet_service),
      delegate_(std::move(delegate)),
      granted_account_(std::move(granted_account)),
      origin_(origin) {
  CHECK(delegate_);
  CHECK(granted_account_);
}

PolkadotApiImpl::~PolkadotApiImpl() = default;

bool PolkadotApiImpl::IsGrantedAccountAllowed() {
  return delegate_->IsAccountAllowed(
      mojom::CoinType::DOT, GetAccountPermissionIdentifier(granted_account_));
}

void PolkadotApiImpl::GetAccounts(bool any_type, GetAccountsCallback callback) {
  // `any_type` is used to filter based on accounts whose underlying keypairs
  // permit derivation. Our Polkadot keyring only supports sr25519, so all of
  // our accounts can always derive further keypairs, which means we can ignore
  // the filtering.
  if (!IsGrantedAccountAllowed()) {
    std::move(callback).Run(std::nullopt, RejectedError());
    return;
  }

  if (keyring_service_->IsLockedSync()) {
    std::move(callback).Run(
        std::nullopt,
        mojom::PolkadotProviderErrorBundle::New(
            mojom::PolkadotProviderError::kUnknown,
            l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR)));
    return;
  }

  auto account = keyring_service_->FindAccount(granted_account_);
  if (!account) {
    std::move(callback).Run(std::nullopt, InternalError());
    return;
  }

  // dApps expect a list of account objects, even though we only permit one.
  std::vector<mojom::PolkadotInjectedAccountPtr> accounts;
  accounts.push_back(MakePolkadotInjectedAccount(*account));
  std::move(callback).Run(std::move(accounts), nullptr);
}

void PolkadotApiImpl::SignPayload(const std::string& payload_json,
                                  SignPayloadCallback callback) {
  if (!IsGrantedAccountAllowed()) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  if (keyring_service_->IsLockedSync()) {
    std::move(callback).Run(
        nullptr,
        mojom::PolkadotProviderErrorBundle::New(
            mojom::PolkadotProviderError::kUnknown,
            l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR)));
    return;
  }

  auto payload = base::JSONReader::ReadDict(
      payload_json, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!payload) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  const auto* address = payload->FindString("address");
  const auto* genesis_hash = payload->FindString("genesisHash");

  // We must have these fields to ensure that the transaction is intended for
  // the granted account and to know which metadata to fetch.
  if (!address || !genesis_hash) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  auto pubkey = keyring_service_->GetPolkadotPubKey(granted_account_);
  auto decoded_address = Ss58Address::Decode(*address);

  // We take the ss58-encoded address passed in the payload and compare it
  // against the pubkey for the granted account. If they mismatch, we know it's
  // safe to reject this transaction.
  if (!pubkey || !decoded_address || decoded_address->public_key != *pubkey) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  std::optional<std::string_view> chain_id =
      PolkadotWalletService::GetChainIdFromGenesisHash(*genesis_hash);

  // If we were given an unknown genesis hash, we can safely reject the
  // transaction.
  if (!chain_id.has_value()) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  auto fetches = base::MakeRefCounted<SignPayloadFetches>();
  auto barrier = base::BarrierClosure(
      2, base::BindOnce(&PolkadotApiImpl::OnSignPayloadFetchesComplete,
                        weak_ptr_factory_.GetWeakPtr(), payload_json,
                        std::string(*chain_id), fetches, std::move(callback)));

  auto* rpc = polkadot_wallet_service_->GetPolkadotRpc();
  rpc->GetMetadata(
      *chain_id,
      base::BindOnce(&PolkadotApiImpl::OnGetMetadataForSignPayload,
                     weak_ptr_factory_.GetWeakPtr(), fetches, barrier));
  rpc->GetSystemProperties(
      *chain_id,
      base::BindOnce(&PolkadotApiImpl::OnGetSystemPropertiesForSignPayload,
                     weak_ptr_factory_.GetWeakPtr(), fetches,
                     std::move(barrier)));
}

PolkadotApiImpl::SignPayloadFetches::SignPayloadFetches() = default;
PolkadotApiImpl::SignPayloadFetches::~SignPayloadFetches() = default;

void PolkadotApiImpl::OnGetMetadataForSignPayload(
    scoped_refptr<SignPayloadFetches> fetches,
    base::RepeatingClosure barrier,
    base::expected<std::vector<uint8_t>, std::string> metadata_bytes) {
  if (metadata_bytes.has_value()) {
    fetches->metadata_bytes = std::move(metadata_bytes.value());
  }
  barrier.Run();
}

void PolkadotApiImpl::OnGetSystemPropertiesForSignPayload(
    scoped_refptr<SignPayloadFetches> fetches,
    base::RepeatingClosure barrier,
    base::expected<mojom::PolkadotChainPropertiesPtr, std::string>
        chain_properties) {
  if (chain_properties.has_value()) {
    fetches->chain_properties = std::move(chain_properties.value());
  }
  barrier.Run();
}

void PolkadotApiImpl::OnSignPayloadFetchesComplete(
    std::string payload_json,
    std::string chain_id,
    scoped_refptr<SignPayloadFetches> fetches,
    SignPayloadCallback callback) {
  // Permission can be revoked while the fetches are in flight.
  if (!IsGrantedAccountAllowed()) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  // One of our network requests failed in some way, just reject the
  // transaction.
  if (!fetches->metadata_bytes || !fetches->chain_properties) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  polkadot_wallet_service_->AddSignPolkadotTransactionRequest(
      mojom::SignPolkadotTransactionRequest::New(
          -1, granted_account_.Clone(), MakeOriginInfo(origin_),
          mojom::ChainId::New(mojom::CoinType::DOT, std::move(chain_id)),
          std::move(payload_json), std::move(*fetches->metadata_bytes),
          std::move(fetches->chain_properties),
          /*signature_payload=*/std::nullopt),
      base::BindOnce(&PolkadotApiImpl::OnSignPayloadRequestProcessed,
                     weak_ptr_factory_.GetWeakPtr(), next_signer_result_id_++,
                     std::move(callback)));

  delegate_->ShowPanel(origin_);
}

void PolkadotApiImpl::OnSignPayloadRequestProcessed(
    int32_t request_id,
    SignPayloadCallback callback,
    bool approved,
    std::optional<std::vector<uint8_t>> signature_payload,
    const std::optional<std::string>& error) {
  if (!approved) {
    // The panel rejects on the user's behalf when it cannot describe a request,
    // and reports why. Blaming the user for that would be a lie.
    std::move(callback).Run(nullptr, error ? InternalError() : RejectedError());
    return;
  }

  // Nothing described the request, so nothing established what approving it
  // meant. The panel gates its Sign button on a description for that reason.
  if (!signature_payload) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  // The user can take long enough over the panel for the account to be revoked,
  // or the wallet to lock, after the request was queued.
  if (!IsGrantedAccountAllowed()) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  auto signature = polkadot_wallet_service_->SignSignaturePayload(
      granted_account_, *signature_payload);
  if (!signature) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  LOG(INFO) << "signature payload as hex is: "
            << base::HexEncodeLower(signature_payload.value());
  LOG(INFO) << "produced signature is: "
            << base::HexEncodeLower(signature.value());

  std::move(callback).Run(
      mojom::PolkadotSignerResult::New(
          request_id, MakePolkadotSignerSignatureHex(*signature)),
      nullptr);
}

}  // namespace brave_wallet
