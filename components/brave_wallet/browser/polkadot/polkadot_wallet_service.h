/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_

#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
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
  using GetChainMetadataCallback = base::OnceCallback<void(
      const base::expected<PolkadotChainMetadata, std::string>&)>;

  using GenerateSignedTransferExtrinsicCallback =
      base::OnceCallback<void(base::expected<std::string, std::string>)>;

  using SignAndSendTransactionCallback =
      base::OnceCallback<void(base::expected<std::string, std::string>)>;

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

  // Get the chain metadata associated with the provided chain_id. Metadata is
  // required for encoding and decoding extrinsics as chains have their own
  // pallet/call indices.
  void GetChainMetadata(std::string_view chain_id,
                        GetChainMetadataCallback callback);

  // Generates a hex-encoded string suitable for sending over the network,
  // signed using the account's private key. The signed extrinsic can be
  // submitted directly using author_submitExtrinsic or payment information can
  // be queried using payment_queryInfo.
  void GenerateSignedTransferExtrinsic(
      std::string_view chain_id,
      const mojom::AccountIdPtr& account_id,
      uint128_t send_amount,
      base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
      GenerateSignedTransferExtrinsicCallback callback);

  void SignAndSendTransaction(
      std::string_view chain_id,
      const mojom::AccountIdPtr& account_id,
      uint128_t send_amount,
      base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
      SignAndSendTransactionCallback callback);

 private:
  struct PolkadotSignTransaction : base::RefCounted<PolkadotSignTransaction> {
    mojom::AccountIdPtr sender_account_id;
    raw_ptr<const PolkadotChainMetadata> chain_metadata;

    std::string chain_id;
    mojom::PolkadotAccountInfoPtr account_info;
    GenerateSignedTransferExtrinsicCallback callback;

    std::optional<PolkadotBlockHeader> chain_header;
    std::optional<PolkadotBlockHeader> finalized_header;
    std::optional<PolkadotBlockHeader> signing_header;

    std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> genesis_hash;
    std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>
        signing_block_hash;

    std::optional<PolkadotRuntimeVersion> runtime_version;

    uint128_t send_amount = 0;
    std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient = {};

    PolkadotSignTransaction();

    bool is_done() const;

   private:
    friend class RefCounted<PolkadotSignTransaction>;
    ~PolkadotSignTransaction();
  };

  // KeyringServiceObserverBase:
  void Unlocked() override;

  void UpdateSigningHeader(
      scoped_refptr<PolkadotSignTransaction> transaction_state);

  // Initialize the metadata structures associated with each specified chain
  // (both mainnet and testnet). This involves RPC calls to the configured
  // remotes, fetching their identifying data and then parsing it and building
  // the chain metadata which encompasses pallet indices and call indices.
  void InitializeChainMetadata();

  // Recreate the routines here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  void GetSigningHeader(
      scoped_refptr<PolkadotSignTransaction> transaction_state);

  // Called by InitializeChainMetadata and is responsible for parsing the
  // network response and updating the nested metadata data members of the
  // PolkadotWalletService.
  void OnInitializeChainMetadata(std::string_view chain_id,
                                 const std::optional<std::string>&,
                                 const std::optional<std::string>&);

  void OnGetMetadataForSigning(
      scoped_refptr<PolkadotSignTransaction> transaction_state,
      const base::expected<PolkadotChainMetadata, std::string>&);

  void OnGetAccount(scoped_refptr<PolkadotSignTransaction> transaction_state,
                    mojom::PolkadotAccountInfoPtr,
                    const std::optional<std::string>&);

  void OnGetChainHeader(
      scoped_refptr<PolkadotSignTransaction> transaction_state,
      std::optional<PolkadotBlockHeader>,
      std::optional<std::string>);

  void OnGetParentHeader(
      scoped_refptr<PolkadotSignTransaction> transaction_state,
      std::optional<PolkadotBlockHeader>,
      std::optional<std::string>);

  void OnGetFinalizedHead(
      scoped_refptr<PolkadotSignTransaction> transaction_state,
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>,
      std::optional<std::string>);

  void OnGetFinalizedBlockHeader(
      scoped_refptr<PolkadotSignTransaction> transaction_state,
      std::optional<PolkadotBlockHeader>,
      std::optional<std::string>);

  void OnGetGenesisHash(
      scoped_refptr<PolkadotSignTransaction> transaction_state,
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>,
      std::optional<std::string>);

  void OnGetSigningBlockHash(
      scoped_refptr<PolkadotSignTransaction> transaction_state,
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>,
      std::optional<std::string>);

  void OnGetRuntimeVersion(
      scoped_refptr<PolkadotSignTransaction> transaction_state,
      std::optional<PolkadotRuntimeVersion>,
      std::optional<std::string>);

  void OnFinalizeSignTransaction(
      scoped_refptr<PolkadotSignTransaction> transaction_state);

  void OnGenerateSignedTransfer(
      std::string chain_id,
      SignAndSendTransactionCallback callback,
      base::expected<std::string, std::string> signed_extrinsic);

  void OnSubmitSignedExtrinsic(SignAndSendTransactionCallback callback,
                               std::optional<std::string>,
                               std::optional<std::string>);

  const raw_ref<KeyringService> keyring_service_;
  mojo::ReceiverSet<mojom::PolkadotWalletService> receivers_;

  std::optional<base::expected<PolkadotChainMetadata, std::string>>
      testnet_chain_metadata_;
  std::optional<base::expected<PolkadotChainMetadata, std::string>>
      mainnet_chain_metadata_;

  std::vector<GetChainMetadataCallback> mainnet_chain_metadata_callbacks_;
  std::vector<GetChainMetadataCallback> testnet_chain_metadata_callbacks_;

  PolkadotSubstrateRpc polkadot_substrate_rpc_;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};

  base::WeakPtrFactory<PolkadotWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
