/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SUBSTRATE_RPC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SUBSTRATE_RPC_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

class NetworkManager;

struct PolkadotBlockHeader {
  std::array<uint8_t, kPolkadotBlockHashSize> parent_hash = {};
  uint32_t block_number = 0;
};

struct PolkadotRuntimeVersion {
  uint32_t spec_version = 0;
  uint32_t transaction_version = 0;
};

// The main driver for the Polkadot-based RPC calls against the relay chain and
// the Substrate-based parachains.
class PolkadotSubstrateRpc {
 public:
  explicit PolkadotSubstrateRpc(
      NetworkManager& network_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~PolkadotSubstrateRpc();

  using GetChainNameCallback =
      base::OnceCallback<void(const std::optional<std::string>&,
                              const std::optional<std::string>&)>;

  using GetAccountBalanceCallback =
      base::OnceCallback<void(mojom::PolkadotAccountInfoPtr,
                              const std::optional<std::string>&)>;

  using GetFinalizedHeadCallback = base::OnceCallback<void(
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>,
      std::optional<std::string>)>;

  using GetBlockHeaderCallback =
      base::OnceCallback<void(std::optional<PolkadotBlockHeader>,
                              std::optional<std::string>)>;

  using GetBlockHashCallback = base::OnceCallback<void(
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>,
      std::optional<std::string>)>;

  using GetRuntimeVersionCallback =
      base::OnceCallback<void(std::optional<PolkadotRuntimeVersion>,
                              std::optional<std::string>)>;

  // Get the name of the chain pointed to by the current network configuration.
  // "Westend" or "Paseo" for the testnets, "Polkadot" for the mainnet.
  void GetChainName(std::string_view chain_id, GetChainNameCallback callback);

  void GetAccountBalance(
      std::string_view chain_id,
      base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> pubkey,
      GetAccountBalanceCallback callback);

  // Get the hash of the last finalized block in the canon chain. This is used
  // during extrinsic creation where the blockhash is used as a portion of the
  // payload. Note, the finalized block's hash is only used when the current
  // head is within the MAX_FINALITY_LAG of the finalized block, as described
  // here:
  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  //
  // This hash defines the start of the mortality period, described as H(B):
  // https://spec.polkadot.network/id-extrinsics#defn-extrinsic-signature
  //
  // This function invokes the provided callback with the raw bytes of the block
  // hash.
  void GetFinalizedHead(std::string_view chain_id,
                        GetFinalizedHeadCallback callback);

  // Get the header for an associated block hash or, if not provided, the header
  // for the latest block in the relay chain. This method is used in tandem with
  // GetFinalizedHead to determine which block hash to use as the start of the
  // mortality period when signing extrinsics. If the lag between the finalized
  // block hash and the current block's parent exceeds the maximum lag time,
  // this block hash is used in the payload that generates the extrinsic
  // signature.
  // See:
  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  // https://spec.polkadot.network/id-extrinsics#defn-extrinsic-signature
  void GetBlockHeader(
      std::string_view chain_id,
      std::optional<base::span<uint8_t, kPolkadotBlockHashSize>> block_hash,
      GetBlockHeaderCallback callback);

  // Get the block hash for a given block number. This is most useful for
  // getting the "genesis hash", which is the blockhash of block 0.
  // If a block number is not provided then the latest block hash is returned.
  // The genesis hash is used to generate the signing payload used during
  // extrinsic creation as outlined by the spec here:
  // https://spec.polkadot.network/id-extrinsics#defn-extrinsic-signature
  void GetBlockHash(std::string_view chain_id,
                    std::optional<uint32_t> block_number,
                    GetBlockHashCallback callback);

  void GetRuntimeVersion(
      std::string_view chain_id,
      std::optional<base::span<uint8_t, kPolkadotBlockHashSize>> block_hash,
      GetRuntimeVersionCallback callback);

 private:
  using APIRequestResult = api_request_helper::APIRequestResult;

  static base::DictValue MakeRpcRequestJson(std::string_view method,
                                            base::ListValue params);

  GURL GetNetworkURL(std::string_view chain_id);

  void OnGetChainName(GetChainNameCallback callback, APIRequestResult res);
  void OnGetAccountBalance(GetAccountBalanceCallback, APIRequestResult res);
  void OnGetFinalizedHead(GetFinalizedHeadCallback, APIRequestResult res);
  void OnGetBlockHeader(GetBlockHeaderCallback callback, APIRequestResult res);
  void OnGetBlockHash(GetBlockHashCallback callback, APIRequestResult res);
  void OnGetRuntimeVersion(GetRuntimeVersionCallback callback,
                           APIRequestResult res);

  const raw_ref<NetworkManager> network_manager_;
  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<PolkadotSubstrateRpc> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SUBSTRATE_RPC_H_
