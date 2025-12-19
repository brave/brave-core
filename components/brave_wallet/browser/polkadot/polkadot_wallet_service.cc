/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

#include "base/types/optional_ref.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {
namespace {

base::expected<PolkadotChainMetadata, std::string> ParseChainMetadataReponse(
    base::optional_ref<const std::string> chain_name,
    base::optional_ref<const std::string> err_str) {
  if (err_str) {
    return base::unexpected(*err_str);
  }

  CHECK(chain_name);
  auto chain_metadata = PolkadotChainMetadata::FromChainName(*chain_name);
  if (!chain_metadata) {
    return base::unexpected(
        "Failed to parse metadata for the provided chain spec.");
  }

  return base::ok(std::move(*chain_metadata));
}

}  // namespace

PolkadotWalletService::PolkadotWalletService(
    KeyringService& keyring_service,
    NetworkManager& network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      polkadot_substrate_rpc_(network_manager, std::move(url_loader_factory)) {
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());

  InitializeChainMetadata();
}

PolkadotWalletService::~PolkadotWalletService() = default;

void PolkadotWalletService::GetChainMetadata(
    std::string_view chain_id,
    GetChainMetadataCallback callback) {
  CHECK(IsPolkadotNetwork(chain_id));
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!testnet_chain_metadata_) {
    // Testnet chain metadata isn't ready yet, defer execution of the callback.
    testnet_chain_metadata_callbacks_.push_back(std::move(callback));
    return;
  }

  if (!mainnet_chain_metadata_) {
    // Mainnet chain metadata isn't ready yet, defer execution of the callback.
    mainnet_chain_metadata_callbacks_.push_back(std::move(callback));
    return;
  }

  // Manually time-slice here so that the QuitClosure + RunUntilQuit idiom
  // works in tests.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&PolkadotWalletService::OnGetChainMetadataPost,
                                weak_ptr_factory_.GetWeakPtr(),
                                std::string(chain_id), std::move(callback)));
}

void PolkadotWalletService::OnGetChainMetadataPost(
    std::string chain_id,
    GetChainMetadataCallback callback) {
  CHECK(IsPolkadotNetwork(chain_id));
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (chain_id == mojom::kPolkadotTestnet) {
    return std::move(callback).Run(*testnet_chain_metadata_);
  }

  if (chain_id == mojom::kPolkadotMainnet) {
    return std::move(callback).Run(*mainnet_chain_metadata_);
  }
}

void PolkadotWalletService::InitializeChainMetadata() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  polkadot_substrate_rpc_.GetChainName(
      mojom::kPolkadotTestnet,
      base::BindOnce(&PolkadotWalletService::OnInitializeChainMetadata,
                     weak_ptr_factory_.GetWeakPtr(), mojom::kPolkadotTestnet));

  polkadot_substrate_rpc_.GetChainName(
      mojom::kPolkadotMainnet,
      base::BindOnce(&PolkadotWalletService::OnInitializeChainMetadata,
                     weak_ptr_factory_.GetWeakPtr(), mojom::kPolkadotMainnet));
}

void PolkadotWalletService::Bind(
    mojo::PendingReceiver<mojom::PolkadotWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void PolkadotWalletService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void PolkadotWalletService::GetNetworkName(mojom::AccountIdPtr account_id,
                                           GetNetworkNameCallback callback) {
  std::string chain_id = GetNetworkForPolkadotAccount(account_id);
  polkadot_substrate_rpc_.GetChainName(std::move(chain_id),
                                       std::move(callback));
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

void PolkadotWalletService::OnInitializeChainMetadata(
    std::string_view chain_id,
    const std::optional<std::string>& chain_name,
    const std::optional<std::string>& err_str) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(IsPolkadotNetwork(chain_id));

  if (chain_id == mojom::kPolkadotTestnet) {
    testnet_chain_metadata_ = ParseChainMetadataReponse(chain_name, err_str);
    for (auto& callback : testnet_chain_metadata_callbacks_) {
      std::move(callback).Run(*testnet_chain_metadata_);
    }
    testnet_chain_metadata_callbacks_.clear();
  }

  if (chain_id == mojom::kPolkadotMainnet) {
    mainnet_chain_metadata_ = ParseChainMetadataReponse(chain_name, err_str);
    for (auto& callback : mainnet_chain_metadata_callbacks_) {
      std::move(callback).Run(*mainnet_chain_metadata_);
    }
    mainnet_chain_metadata_callbacks_.clear();
  }
}

}  // namespace brave_wallet
