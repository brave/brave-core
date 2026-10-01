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

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
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
    BraveWalletService& brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    mojom::AccountIdPtr granted_account,
    const url::Origin& origin)
    : brave_wallet_service_(brave_wallet_service),
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

  auto* keyring_service = brave_wallet_service_->keyring_service();
  if (keyring_service->IsLockedSync()) {
    std::move(callback).Run(
        std::nullopt,
        mojom::PolkadotProviderErrorBundle::New(
            mojom::PolkadotProviderError::kUnknown,
            l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR)));
    return;
  }

  auto account = keyring_service->FindAccount(granted_account_);
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

  auto* keyring_service = brave_wallet_service_->keyring_service();
  if (keyring_service->IsLockedSync()) {
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

  const std::string* address = payload->FindString("address");
  const std::string* genesis_hash_hex = payload->FindString("genesisHash");
  if (!address || !genesis_hash_hex) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  // A dapp can name any address it likes in the payload, so pin it to the one
  // account it was granted before the user is shown anything. The ss58 prefix
  // is ignored: the same key is spelled differently per chain.
  auto pubkey = keyring_service->GetPolkadotPubKey(granted_account_);
  auto decoded_address = Ss58Address::Decode(*address);
  if (!pubkey || !decoded_address || decoded_address->public_key != *pubkey) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  std::string_view genesis_hash_str = *genesis_hash_hex;
  if (base::StartsWith(genesis_hash_str, "0x",
                       base::CompareCase::INSENSITIVE_ASCII)) {
    genesis_hash_str.remove_prefix(2);
  }

  std::array<uint8_t, kPolkadotBlockHashSize> genesis_hash = {};
  if (!base::HexStringToSpan(genesis_hash_str, genesis_hash)) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  brave_wallet_service_->GetPolkadotWalletService()
      ->ResolveChainIdByGenesisHash(
          granted_account_, genesis_hash,
          base::BindOnce(&PolkadotApiImpl::OnResolveChainIdForSignPayload,
                         weak_ptr_factory_.GetWeakPtr(), payload_json,
                         std::move(callback)));
}

void PolkadotApiImpl::OnResolveChainIdForSignPayload(
    std::string payload_json,
    SignPayloadCallback callback,
    std::optional<std::string> chain_id) {
  // A genesis hash we can't place is a chain we have no endpoint for, so we
  // can't fetch its metadata and can't describe what we'd be signing.
  if (!chain_id) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  auto* rpc =
      brave_wallet_service_->GetPolkadotWalletService()->GetPolkadotRpc();
  rpc->GetMetadata(
      *chain_id,
      base::BindOnce(&PolkadotApiImpl::OnGetMetadataForSignPayload,
                     weak_ptr_factory_.GetWeakPtr(), std::move(payload_json),
                     *chain_id, std::move(callback)));
}

void PolkadotApiImpl::OnGetMetadataForSignPayload(
    std::string payload_json,
    std::string chain_id,
    SignPayloadCallback callback,
    base::expected<std::vector<uint8_t>, std::string> metadata_bytes) {
  // Permission can be revoked while the metadata fetch is in flight.
  if (!IsGrantedAccountAllowed()) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  if (!metadata_bytes.has_value()) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  // The metadata describes the call's shape but not the units it's denominated
  // in, so the chain's properties are fetched before the request is queued: the
  // panel describes a request once, and describing it in the wrong token or
  // address format is worse than not describing it at all.
  auto* rpc =
      brave_wallet_service_->GetPolkadotWalletService()->GetPolkadotRpc();
  rpc->GetSystemProperties(
      chain_id,
      base::BindOnce(&PolkadotApiImpl::OnGetSystemPropertiesForSignPayload,
                     weak_ptr_factory_.GetWeakPtr(), std::move(payload_json),
                     chain_id, std::move(metadata_bytes.value()),
                     std::move(callback)));
}

void PolkadotApiImpl::OnGetSystemPropertiesForSignPayload(
    std::string payload_json,
    std::string chain_id,
    std::vector<uint8_t> metadata_bytes,
    SignPayloadCallback callback,
    base::expected<mojom::PolkadotChainPropertiesPtr, std::string>
        chain_properties) {
  // Permission can be revoked while the properties fetch is in flight.
  if (!IsGrantedAccountAllowed()) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  if (!chain_properties.has_value()) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  brave_wallet_service_->AddSignPolkadotTransactionRequest(
      mojom::SignPolkadotTransactionRequest::New(
          -1, granted_account_.Clone(), MakeOriginInfo(origin_),
          mojom::ChainId::New(mojom::CoinType::DOT, std::move(chain_id)),
          std::move(payload_json), std::move(metadata_bytes),
          std::move(chain_properties.value()),
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
    std::move(callback).Run(nullptr, RejectedError());
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

  auto signature =
      brave_wallet_service_->GetPolkadotWalletService()->SignSignaturePayload(
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
