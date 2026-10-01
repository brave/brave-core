/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_api_impl.h"

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/barrier_closure.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

constexpr std::string_view kAddress = "address";
constexpr std::string_view kGenesisHash = "genesisHash";
constexpr std::string_view kSpecVersion = "specVersion";

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
      dapp_origin_(origin) {
  CHECK(delegate_);
  CHECK(granted_account_);
  CHECK(IsValidDappOrigin(dapp_origin_));
}

PolkadotApiImpl::~PolkadotApiImpl() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  polkadot_wallet_service_->ClearCancelledPendingSignRequests();
}

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

  const auto* address = payload->FindString(kAddress);
  const auto* genesis_hash = payload->FindString(kGenesisHash);

  // We must have these fields to ensure that the transaction is intended for
  // the granted account and for a chain we know.
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

  auto fetches = std::make_unique<SignPayloadFetches>();
  auto* fetches_data = fetches.get();
  auto barrier = base::BarrierClosure(
      3, base::BindOnce(&PolkadotApiImpl::OnSignPayloadFetchesComplete,
                        weak_ptr_factory_.GetWeakPtr(),
                        std::move(payload.value()), std::string(*chain_id),
                        base::Owned(std::move(fetches)), std::move(callback)));

  auto* rpc = polkadot_wallet_service_->GetPolkadotRpc();
  rpc->GetMetadata(
      *chain_id,
      base::BindOnce(&PolkadotApiImpl::OnGetMetadataForSignPayload,
                     weak_ptr_factory_.GetWeakPtr(), fetches_data, barrier));

  rpc->GetSystemProperties(
      *chain_id,
      base::BindOnce(&PolkadotApiImpl::OnGetSystemPropertiesForSignPayload,
                     weak_ptr_factory_.GetWeakPtr(), fetches_data, barrier));

  rpc->GetRuntimeVersion(
      *chain_id, std::nullopt,
      base::BindOnce(&PolkadotApiImpl::OnGetRuntimeVersionForSignPayload,
                     weak_ptr_factory_.GetWeakPtr(), fetches_data, barrier));
}

PolkadotApiImpl::SignPayloadFetches::SignPayloadFetches() = default;
PolkadotApiImpl::SignPayloadFetches::~SignPayloadFetches() = default;

void PolkadotApiImpl::OnGetMetadataForSignPayload(
    SignPayloadFetches* fetches,
    base::RepeatingClosure barrier,
    base::expected<std::vector<uint8_t>, std::string> metadata_bytes) {
  if (metadata_bytes.has_value()) {
    fetches->metadata_bytes = std::move(metadata_bytes.value());
  }
  barrier.Run();
}

void PolkadotApiImpl::OnGetSystemPropertiesForSignPayload(
    SignPayloadFetches* fetches,
    base::RepeatingClosure barrier,
    base::expected<mojom::PolkadotChainPropertiesPtr, std::string>
        chain_properties) {
  if (chain_properties.has_value()) {
    fetches->chain_properties = std::move(chain_properties.value());
  }
  barrier.Run();
}

void PolkadotApiImpl::OnGetRuntimeVersionForSignPayload(
    SignPayloadFetches* fetches,
    base::RepeatingClosure barrier,
    std::optional<PolkadotRuntimeVersion> runtime_version,
    std::optional<std::string> error_message) {
  if (runtime_version.has_value()) {
    fetches->runtime_version = runtime_version.value();
  }
  barrier.Run();
}

void PolkadotApiImpl::OnSignPayloadFetchesComplete(
    base::DictValue payload,
    std::string chain_id,
    SignPayloadFetches* fetches,
    SignPayloadCallback callback) {
  // Permission can be revoked while the fetches are in flight.
  if (!IsGrantedAccountAllowed()) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  // One of our network requests failed in some way, just reject the
  // transaction.
  if (!fetches->metadata_bytes || !fetches->chain_properties ||
      !fetches->runtime_version) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  const auto* spec_version = payload.FindString(kSpecVersion);
  uint32_t version = 0;

  // specVersion is a required SignerPayloadJSON field. Must begin with a "0x".
  if (!spec_version || !spec_version->starts_with("0x")) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  if (!base::HexStringToUInt(std::string_view(*spec_version).substr(2),
                             &version)) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  // If the specVersion associated with the payload is stale, we can safely
  // reject.
  if (version != fetches->runtime_version->spec_version) {
    std::move(callback).Run(nullptr, RejectedError());
    return;
  }

  auto payload_json = base::WriteJson(payload);
  if (!payload_json.has_value()) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  polkadot_wallet_service_->AddSignPolkadotTransactionRequest(
      mojom::SignPolkadotTransactionRequest::New(
          0, granted_account_.Clone(), MakeDappOriginInfo(dapp_origin_),
          mojom::ChainId::New(mojom::CoinType::DOT, std::move(chain_id)),
          std::move(payload_json.value()), std::move(*fetches->metadata_bytes),
          std::move(fetches->chain_properties),
          /*signature_payload=*/std::nullopt),
      base::BindOnce(&PolkadotApiImpl::OnSignPayloadRequestProcessed,
                     weak_ptr_factory_.GetWeakPtr(), next_signer_result_id_++,
                     std::move(callback)));

  delegate_->ShowPanel(dapp_origin_);
}

void PolkadotApiImpl::OnSignPayloadRequestProcessed(
    int32_t request_id,
    SignPayloadCallback callback,
    bool approved,
    std::optional<std::vector<uint8_t>> signature_payload,
    const std::optional<std::string>& error) {
  if (!approved) {
    // The panel automatically rejects a request when, for example, decoding the
    // transaction fails. If we're supplied an error message, we invoke the
    // callback with InternalError() instead of RejectedError().
    std::move(callback).Run(nullptr, error ? InternalError() : RejectedError());
    return;
  }

  // We cannot proceed without a valid signature payload.
  if (!signature_payload || signature_payload->empty()) {
    std::move(callback).Run(nullptr, InternalError());
    return;
  }

  // We require the account to still currently hold permissions.
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
  // The Signer interface requires us to include the request id, which we count
  // using a per-Injected basis.
  // https://github.com/polkadot-js/api/blob/81aabcce128f7f8ca5cf83e4a4d5705855950ca9/packages/types/src/types/extrinsic.ts#L144-L166
  std::move(callback).Run(
      mojom::PolkadotSignerResult::New(
          request_id, MakePolkadotSignerSignatureHex(*signature)),
      nullptr);
}

}  // namespace brave_wallet
