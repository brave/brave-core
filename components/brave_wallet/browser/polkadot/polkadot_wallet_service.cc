/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

#include <algorithm>

#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

PolkadotWalletService::PolkadotWalletService(
    KeyringService& keyring_service,
    NetworkManager& network_manager,
    ::PrefService& profile_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      network_manager_(network_manager),
      polkadot_substrate_rpc_(network_manager, std::move(url_loader_factory)),
      chain_metadata_prefs_(profile_prefs),
      metadata_provider_(chain_metadata_prefs_, polkadot_substrate_rpc_) {
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
}

PolkadotWalletService::~PolkadotWalletService() = default;

void PolkadotWalletService::GetChainMetadata(
    std::string_view chain_id,
    GetChainMetadataCallback callback) {
  CHECK(IsPolkadotNetwork(chain_id));
  metadata_provider_.GetChainMetadata(chain_id, std::move(callback));
}

void PolkadotWalletService::Bind(
    mojo::PendingReceiver<mojom::PolkadotWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void PolkadotWalletService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void PolkadotWalletService::Unlocked() {
  metadata_provider_.Init();
}

PolkadotSubstrateRpc* PolkadotWalletService::GetPolkadotRpc() {
  return &polkadot_substrate_rpc_;
}

void PolkadotWalletService::GetNetworkName(mojom::AccountIdPtr account_id,
                                           GetNetworkNameCallback callback) {
  std::string chain_id = GetNetworkForPolkadotAccount(account_id);
  polkadot_substrate_rpc_.GetChainName(std::move(chain_id),
                                       std::move(callback));
}

void PolkadotWalletService::GetCompatibleNetworks(
    mojom::AccountIdPtr account_id,
    GetCompatibleNetworksCallback callback) {
  if (!account_id || account_id->coin != mojom::CoinType::DOT ||
      !IsPolkadotKeyring(account_id->keyring_id)) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  const std::string compatible_chain_id =
      GetNetworkForPolkadotKeyring(account_id->keyring_id);
  const auto hidden_networks =
      network_manager_->GetHiddenNetworks(mojom::CoinType::DOT);

  std::vector<mojom::NetworkInfoPtr> compatible_networks;
  for (auto& network_info : network_manager_->GetAllChains()) {
    if (network_info->coin != mojom::CoinType::DOT) {
      continue;
    }

    if (network_info->chain_id == compatible_chain_id &&
        !std::ranges::contains(hidden_networks,
                               base::ToLowerASCII(network_info->chain_id))) {
      compatible_networks.push_back(std::move(network_info));
    }
  }

  std::move(callback).Run(std::move(compatible_networks));
}

void PolkadotWalletService::GetAddress(mojom::AccountIdPtr account_id,
                                       const std::string& chain_id,
                                       GetAddressCallback callback) {
  if (!account_id || account_id->coin != mojom::CoinType::DOT ||
      !IsPolkadotKeyring(account_id->keyring_id) ||
      !IsPolkadotNetwork(chain_id) ||
      GetNetworkForPolkadotKeyring(account_id->keyring_id) != chain_id) {
    std::move(callback).Run(std::nullopt, WalletInternalErrorMessage());
    return;
  }

  auto pubkey = keyring_service_->GetPolkadotPubKey(account_id);
  if (!pubkey) {
    std::move(callback).Run(std::nullopt, WalletInternalErrorMessage());
    return;
  }

  const uint16_t prefix =
      chain_id == mojom::kPolkadotMainnet ? kPolkadotPrefix : kWestendPrefix;
  PolkadotAddress polkadot_address;
  polkadot_address.pubkey = *pubkey;
  polkadot_address.ss58_prefix = prefix;
  auto address = polkadot_address.ToString();
  if (!address) {
    std::move(callback).Run(std::nullopt, WalletInternalErrorMessage());
    return;
  }

  std::move(callback).Run(std::move(*address), std::nullopt);
}

void PolkadotWalletService::GetAccountBalance(
    mojom::AccountIdPtr account_id,
    const std::string& chain_id,
    GetAccountBalanceCallback callback) {
  auto pubkey = keyring_service_->GetPolkadotPubKey(account_id);
  if (!pubkey) {
    return std::move(callback).Run(nullptr, WalletInternalErrorMessage());
  }

  polkadot_substrate_rpc_.GetAccountBalance(chain_id, *pubkey,
                                            std::move(callback));
}

void PolkadotWalletService::GenerateSignedTransferExtrinsicImpl(
    std::string chain_id,
    mojom::AccountIdPtr account_id,
    bool use_dummy_signature,
    std::variant<uint128_t, TransferAll> transfer_amount,
    base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
    GenerateSignedTransferExtrinsicCallback callback) {
  auto pubkey = keyring_service_->GetPolkadotPubKey(account_id);
  if (!pubkey) {
    return std::move(callback).Run(
        base::unexpected(WalletInternalErrorMessage()));
  }

  auto transaction_state = std::make_unique<PolkadotSignedTransferTask>(
      *this, *keyring_service_, std::move(account_id), std::move(chain_id),
      use_dummy_signature, std::move(transfer_amount), *pubkey, recipient);

  auto& transaction = *transaction_state;

  auto [pos, inserted] =
      polkadot_sign_transactions_.insert(std::move(transaction_state));
  CHECK(inserted);

  transaction.Start(base::BindOnce(
      &PolkadotWalletService::OnGenerateSignedTransferExtrinsic,
      weak_ptr_factory_.GetWeakPtr(), pos->get(), std::move(callback)));
}

void PolkadotWalletService::GenerateSignedTransferExtrinsic(
    std::string chain_id,
    mojom::AccountIdPtr account_id,
    std::variant<uint128_t, TransferAll> transfer_amount,
    base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
    GenerateSignedTransferExtrinsicCallback callback) {
  GenerateSignedTransferExtrinsicImpl(
      std::move(chain_id), std::move(account_id), false,
      std::move(transfer_amount), recipient, std::move(callback));
}

void PolkadotWalletService::OnGenerateSignedTransferExtrinsic(
    PolkadotSignedTransferTask* transaction_state,
    GenerateSignedTransferExtrinsicCallback callback,
    base::expected<PolkadotExtrinsicMetadata, std::string> extrinsic_metadata) {
  polkadot_sign_transactions_.erase(transaction_state);
  std::move(callback).Run(std::move(extrinsic_metadata));
}

void PolkadotWalletService::SignAndSendTransaction(
    std::string chain_id,
    mojom::AccountIdPtr account_id,
    std::variant<uint128_t, TransferAll> transfer_amount,
    base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
    SignAndSendTransactionCallback callback) {
  GenerateSignedTransferExtrinsic(
      chain_id, std::move(account_id), std::move(transfer_amount), recipient,
      base::BindOnce(&PolkadotWalletService::OnGenerateSignedTransfer,
                     weak_ptr_factory_.GetWeakPtr(), chain_id,
                     std::move(callback)));
}

void PolkadotWalletService::OnGenerateSignedTransfer(
    std::string chain_id,
    SignAndSendTransactionCallback callback,
    base::expected<PolkadotExtrinsicMetadata, std::string> extrinsic_metadata) {
  if (!extrinsic_metadata.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(extrinsic_metadata.error())));
  }

  auto extrinsic = base::HexEncodeLower(extrinsic_metadata->extrinsic());
  polkadot_substrate_rpc_.SubmitExtrinsic(
      chain_id, extrinsic,
      base::BindOnce(&PolkadotWalletService::OnSubmitSignedExtrinsic,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(extrinsic_metadata.value()),
                     std::move(callback)));
}

void PolkadotWalletService::OnSubmitSignedExtrinsic(
    PolkadotExtrinsicMetadata extrinsic_metadata,
    SignAndSendTransactionCallback callback,
    std::optional<std::string> transaction_hash,
    std::optional<std::string> error_str) {
  if (error_str) {
    return std::move(callback).Run(base::unexpected(*error_str));
  }

  CHECK(transaction_hash.has_value());
  std::move(callback).Run(base::ok(std::make_pair(
      std::move(transaction_hash.value()), std::move(extrinsic_metadata))));
}

void PolkadotWalletService::GetFeeEstimate(
    std::string chain_id,
    mojom::AccountIdPtr account_id,
    std::variant<uint128_t, TransferAll> transfer_amount,
    base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
    GetFeeEstimateCallback callback) {
  GenerateSignedTransferExtrinsicImpl(
      chain_id, std::move(account_id), true, std::move(transfer_amount),
      recipient,
      base::BindOnce(&PolkadotWalletService::OnGenerateTransferForFee,
                     weak_ptr_factory_.GetWeakPtr(), chain_id,
                     std::move(callback)));
}

void PolkadotWalletService::OnGenerateTransferForFee(
    std::string chain_id,
    GetFeeEstimateCallback callback,
    base::expected<PolkadotExtrinsicMetadata, std::string> metadata) {
  if (!metadata.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(metadata.error())));
  }

  polkadot_substrate_rpc_.GetPaymentInfo(
      chain_id, metadata->extrinsic(),
      base::BindOnce(&PolkadotWalletService::OnEstimatedFee,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotWalletService::OnEstimatedFee(
    GetFeeEstimateCallback callback,
    base::expected<uint128_t, std::string> partial_fee) {
  std::move(callback).Run(std::move(partial_fee));
}

}  // namespace brave_wallet
