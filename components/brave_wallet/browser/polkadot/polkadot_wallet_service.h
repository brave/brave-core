/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_

#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_signed_transfer_task.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace brave_wallet {

class KeyringService;
class NetworkManager;

// The main Polkadot-based interface that the front-end interacts with.
class PolkadotWalletService : public mojom::PolkadotWalletService,
                              public KeyringServiceObserverBase {
 public:
  using GetChainMetadataCallback = base::OnceCallback<void(
      base::expected<PolkadotChainMetadata, std::string>)>;

  using GenerateSignedTransferExtrinsicCallback = base::OnceCallback<void(
      base::expected<std::vector<uint8_t>, std::string>)>;

  PolkadotWalletService(
      KeyringService& keyring_service,
      NetworkManager& network_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  ~PolkadotWalletService() override;

  // Adds a PolkadotWalletService to the internal ReceiverSet.
  void Bind(mojo::PendingReceiver<mojom::PolkadotWalletService> receiver);

  // Invalidates all the weak ptrs in use by this service.
  void Reset();

  PolkadotSubstrateRpc* GetPolkadotRpc();

  // Get the name of the chain currently pointed to by the current network
  // configuration.
  void GetNetworkName(mojom::AccountIdPtr account_id,
                      GetNetworkNameCallback callback) override;

  void GetAccountBalance(mojom::AccountIdPtr account,
                         const std::string& chain_id,
                         GetAccountBalanceCallback callback) override;

  // Get the chain metadata associated with the provided chain_id. Metadata is
  // required for encoding and decoding extrinsics as chains have their own
  // pallet/call indices.
  void GetChainMetadata(std::string_view chain_id,
                        GetChainMetadataCallback callback);

  // Generates an encoded byte array representing a transfer_allow_death call,
  // suitable for sending over the network as hex, signed using the account's
  // private key. The signed extrinsic can be submitted directly using
  // author_submitExtrinsic or payment information can be queried using
  // payment_queryInfo.
  void GenerateSignedTransferExtrinsic(
      std::string chain_id,
      mojom::AccountIdPtr account_id,
      uint128_t send_amount,
      base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
      GenerateSignedTransferExtrinsicCallback callback);

 private:
  // KeyringServiceObserverBase:
  void Unlocked() override;

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

  void OnGenerateSignedTransferExtrinsic(
      PolkadotSignedTransferTask* transaction_state,
      GenerateSignedTransferExtrinsicCallback callback,
      base::expected<std::vector<uint8_t>, std::string> signed_extrinsic);

  const raw_ref<KeyringService> keyring_service_;
  mojo::ReceiverSet<mojom::PolkadotWalletService> receivers_;

  std::optional<base::expected<PolkadotChainMetadata, std::string>>
      testnet_chain_metadata_;
  std::optional<base::expected<PolkadotChainMetadata, std::string>>
      mainnet_chain_metadata_;

  std::vector<GetChainMetadataCallback> mainnet_chain_metadata_callbacks_;
  std::vector<GetChainMetadataCallback> testnet_chain_metadata_callbacks_;

  PolkadotSubstrateRpc polkadot_substrate_rpc_;
  absl::flat_hash_set<std::unique_ptr<PolkadotSignedTransferTask>>
      polkadot_sign_transactions_;

  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};

  base::WeakPtrFactory<PolkadotWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
