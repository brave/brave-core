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

  using GetFinalizedHeadCallback =
      base::OnceCallback<void(std::optional<std::string>,
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
  // This function invokes the provided callback with the hex-encoded hash from
  // the RPC nodes, but with the leading `0x` removed for the sake of
  // convenience and composability when assembling the final payload for
  // signing.
  void GetFinalizedHead(std::string_view chain_id,
                        GetFinalizedHeadCallback callback);

 private:
  using APIRequestResult = api_request_helper::APIRequestResult;

  static base::DictValue MakeRpcRequestJson(std::string_view method,
                                            base::ListValue params);

  GURL GetNetworkURL(std::string_view chain_id);
  void OnGetChainName(GetChainNameCallback callback, APIRequestResult res);
  void OnGetAccountBalance(GetAccountBalanceCallback, APIRequestResult res);
  void OnGetFinalizedHead(GetFinalizedHeadCallback, APIRequestResult res);

  const raw_ref<NetworkManager> network_manager_;
  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<PolkadotSubstrateRpc> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_SUBSTRATE_RPC_H_
