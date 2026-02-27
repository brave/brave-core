/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata_prefs.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_metadata_provider.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_signed_transfer_task.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

class PrefService;

namespace brave_wallet {

class KeyringService;
class NetworkManager;

// The main Polkadot-based interface that the front-end interacts with.
class PolkadotWalletService : public mojom::PolkadotWalletService {
 public:
  using GetChainMetadataCallback = base::OnceCallback<void(
      base::expected<PolkadotChainMetadata, std::string>)>;

  using GenerateSignedTransferExtrinsicCallback = base::OnceCallback<void(
      base::expected<PolkadotExtrinsicMetadata, std::string>)>;

  using SignAndSendTransactionCallback = base::OnceCallback<void(
      base::expected<std::pair<std::string, PolkadotExtrinsicMetadata>,
                     std::string>)>;

  PolkadotWalletService(
      KeyringService& keyring_service,
      NetworkManager& network_manager,
      ::PrefService* profile_prefs,
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

  void SignAndSendTransaction(
      std::string chain_id,
      mojom::AccountIdPtr account_id,
      uint128_t send_amount,
      base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
      SignAndSendTransactionCallback callback);

  using GetFeeEstimateCallback =
      base::OnceCallback<void(base::expected<uint128_t, std::string>)>;

  void GetFeeEstimate(
      std::string chain_id,
      mojom::AccountIdPtr account_id,
      uint128_t send_amount,
      base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
      GetFeeEstimateCallback callback);

 private:
  void GenerateSignedTransferExtrinsicImpl(
      std::string chain_id,
      mojom::AccountIdPtr account_id,
      bool use_dummy_signature,
      uint128_t send_amount,
      base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
      GenerateSignedTransferExtrinsicCallback callback);
  void OnGenerateSignedTransferExtrinsic(
      PolkadotSignedTransferTask* transaction_state,
      GenerateSignedTransferExtrinsicCallback callback,
      base::expected<PolkadotExtrinsicMetadata, std::string>
          extrinsic_metadata);

  void OnGenerateSignedTransfer(std::string chain_id,
                                SignAndSendTransactionCallback callback,
                                base::expected<PolkadotExtrinsicMetadata,
                                               std::string> extrinsic_metadata);

  void OnSubmitSignedExtrinsic(PolkadotExtrinsicMetadata extrinsic_metadata,
                               SignAndSendTransactionCallback callback,
                               std::optional<std::string>,
                               std::optional<std::string>);

  void OnGenerateTransferForFee(std::string chain_id,
                                GetFeeEstimateCallback callback,
                                base::expected<PolkadotExtrinsicMetadata,
                                               std::string> extrinsic_metadata);

  void OnEstimatedFee(GetFeeEstimateCallback callback,
                      base::expected<uint128_t, std::string> partial_fee);

  const raw_ref<KeyringService> keyring_service_;
  mojo::ReceiverSet<mojom::PolkadotWalletService> receivers_;

  PolkadotSubstrateRpc polkadot_substrate_rpc_;
  PolkadotChainMetadataPrefs chain_metadata_prefs_;
  PolkadotMetadataProvider metadata_provider_;
  absl::flat_hash_set<std::unique_ptr<PolkadotSignedTransferTask>>
      polkadot_sign_transactions_;

  base::WeakPtrFactory<PolkadotWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
