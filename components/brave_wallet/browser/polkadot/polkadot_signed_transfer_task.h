/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SIGNED_TRANSFER_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SIGNED_TRANSFER_TASK_H_

#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class PolkadotWalletService;
class KeyringService;

struct PolkadotSignedTransferTask {
 public:
  using GenerateSignedTransferExtrinsicCallback =
      base::OnceCallback<void(base::expected<std::string, std::string>)>;

  PolkadotSignedTransferTask(
      PolkadotWalletService& polkadot_wallet_service,
      KeyringService& keyring_service,
      mojom::AccountIdPtr sender_account_id,
      std::string_view chain_id,
      uint128_t send_amount,
      base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> sender,
      base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> recipient);

  ~PolkadotSignedTransferTask();

  void Start(GenerateSignedTransferExtrinsicCallback callback);

 private:
  void StopWithError(std::string error_string);

  bool IsDone() const;

  PolkadotSubstrateRpc* GetPolkadotRpc();

  // Signed extrinsics require a nonce from the sender, which we can pull by
  // querying the account information.
  // https://spec.polkadot.network/id-extrinsics#defn-extra-data
  void GetNonce();

  // Signed extrinsics require the hash of the block chain at genesis which we
  // can trivially fetch using RPC calls.
  // https://spec.polkadot.network/id-extrinsics#defn-extrinsic-signature
  void GetGenesisHash();

  // Extrinsic signing requires choosing a block to start the mortality
  // period, whose header we must obtain. Recreate the routines used by
  // polkadot-js where the canonical finalized head is compared to the latest
  // head:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  void GetSigningHeader();

  // Internal helper called by the independent network fetches that retrieve
  // the latest finalized head and the latest head in the block chain.
  void UpdateSigningHeader();

  // Associates the chain metadata (pallet indices, call indices) with the
  // current in-flight transaction.
  void OnGetMetadataForSigning(
      const base::expected<PolkadotChainMetadata, std::string>&);

  // Associate the account information with the current in-flight transaction.
  // Note that only the nonce is currently used but it may prove useful later
  // to have access to the kinds of funds the sender has.
  void OnGetAccountNonce(mojom::PolkadotAccountInfoPtr,
                         const std::optional<std::string>&);

  // Fetch the header of the latest block in the chain.
  // https://github.com/w3f/PSPs/blob/b6d570173146e7a012cf43d270177e02ed886e2e/PSPs/drafts/psp-6.md#192-chain_getheader
  void OnGetChainHeader(std::optional<PolkadotBlockHeader>,
                        std::optional<std::string>);

  // Internal helper used a follow-up for getting the chain's latest header.
  // This parent header's block hash may need to be fetched later.
  void OnGetParentHeader(std::optional<PolkadotBlockHeader>,
                         std::optional<std::string>);

  // Internal helper used to take the block hash of the finalized head and
  // then fetch the block header itself.
  void OnGetFinalizedHead(
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>,
      std::optional<std::string>);

  // Internal helper used to associated the finalized header with the current
  // in-flight transaction.
  void OnGetFinalizedBlockHeader(std::optional<PolkadotBlockHeader>,
                                 std::optional<std::string>);

  // Internal helper used to associate the chain's genesis hash with the
  // current in-flight transaction.
  void OnGetGenesisHash(
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>,
      std::optional<std::string>);

  // Internal helper used to associate the block hash of the block that marks
  // the start of the mortality period (either the latest head or finalized).
  void OnGetSigningBlockHash(
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>,
      std::optional<std::string>);

  // Internal helper that associates the spec version and transaction version
  // with the in-flight transaction.
  void OnGetRuntimeVersion(std::optional<PolkadotRuntimeVersion>,
                           std::optional<std::string>);

  // The finalization process that actually generates the signature payload,
  // signs it using the sender's private key and then invoking the user's
  // callback with the result of the operation.
  void OnFinalizeSignTransaction();

  base::raw_ref<PolkadotWalletService> polkadot_wallet_service_;
  base::raw_ref<KeyringService> keyring_service_;
  mojom::AccountIdPtr sender_account_id_;
  std::string chain_id_;
  uint128_t send_amount_ = 0;
  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> sender_ = {};
  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_ = {};
  GenerateSignedTransferExtrinsicCallback callback_;

  raw_ptr<const PolkadotChainMetadata> chain_metadata_;

  mojom::PolkadotAccountInfoPtr account_info_;

  std::optional<PolkadotBlockHeader> chain_header_;
  std::optional<PolkadotBlockHeader> finalized_header_;
  std::optional<PolkadotBlockHeader> signing_header_;

  std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> genesis_hash_;
  std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>
      signing_block_hash_;

  std::optional<PolkadotRuntimeVersion> runtime_version_;

  base::WeakPtrFactory<PolkadotSignedTransferTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SIGNED_TRANSFER_TASK_H_
