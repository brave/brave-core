/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_

#include "base/sequence_checker.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

class KeyringService;
class NetworkManager;

// The main Polkadot-based interface that the front-end interacts with.
class PolkadotWalletService : public mojom::PolkadotWalletService,
                              public KeyringServiceObserverBase {
 public:
  PolkadotWalletService(
      KeyringService& keyring_service,
      NetworkManager& network_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  ~PolkadotWalletService() override;

  // Adds a PolkadotWalletService to the internal ReceiverSet.
  void Bind(mojo::PendingReceiver<mojom::PolkadotWalletService> receiver);

  // Invalidates all the weak ptrs in use by this service.
  void Reset();

  // Get the name of the chain currently pointed to by the current network
  // configuration.
  void GetNetworkName(mojom::AccountIdPtr account_id,
                      GetNetworkNameCallback callback) override;

  void GetAccountBalance(mojom::AccountIdPtr account,
                         const std::string& chain_id,
                         GetAccountBalanceCallback callback) override;

  // Public-facing helper designed to tell the calling code if the metadata
  // structures have been initialized for the configured mainnet and testnet
  // endpoints.
  bool IsInitialized() const;

  // Obtain a const reference to the initialized testnet chain metadata.
  const base::expected<PolkadotChainMetadata, std::string>&
  testnet_chain_metadata() const {
    CHECK(testnet_chain_metadata_);
    return *testnet_chain_metadata_;
  }

  // Obtain a const reference to the initialized mainnet chain metadata.
  const base::expected<PolkadotChainMetadata, std::string>&
  mainnet_chain_metadata() const {
    CHECK(mainnet_chain_metadata_);
    return *mainnet_chain_metadata_;
  }

 private:
  // Initialize the metadata structures associated with each specified chain
  // (both mainnet and testnet). This involves RPC calls to the configured
  // remotes, fetching their identifying data and then parsing it and building
  // the chain metadata which encompasses pallet indices and call indices.
  void InitializeChainMetadata();

  // Called by InitializeChainMetadata and is responsible for parsing the
  // network response and updating the nested metadata data members of the
  // PolkadotWalletService.
  void OnInitializeChainMetadata(std::string_view chain_id,
                                 const std::optional<std::string>&,
                                 const std::optional<std::string>&);

  SEQUENCE_CHECKER(sequence_checker_);

  const raw_ref<KeyringService> keyring_service_;
  mojo::ReceiverSet<mojom::PolkadotWalletService> receivers_;

  std::optional<base::expected<PolkadotChainMetadata, std::string>>
      testnet_chain_metadata_;
  std::optional<base::expected<PolkadotChainMetadata, std::string>>
      mainnet_chain_metadata_;

  PolkadotSubstrateRpc polkadot_substrate_rpc_;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};

  base::WeakPtrFactory<PolkadotWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
