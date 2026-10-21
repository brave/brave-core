/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_REGISTRY_H_

#include <optional>
#include <string>
#include <vector>

#include "base/sequence_checker.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace base {
template <typename T>
class NoDestructor;
class FilePath;
}  // namespace base

namespace brave_wallet {

class BlockchainRegistry : public mojom::BlockchainRegistry {
 public:
  static constexpr char kRestrictedAddressFileName[] =
      "prohibited-addresses.json";
  static constexpr char kPolkadotRpcUrlsFileName[] = "polkadot-rpc-urls.json";

  BlockchainRegistry(const BlockchainRegistry&) = delete;
  BlockchainRegistry& operator=(const BlockchainRegistry&) = delete;

  static BlockchainRegistry* GetInstance();
  mojo::PendingRemote<mojom::BlockchainRegistry> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BlockchainRegistry> receiver);

  void UpdateCoingeckoIdsMap(CoingeckoIdsMap coingecko_ids_map);
  void UpdateTokenList(TokenListMap tokens);
  void UpdateTokenList(const std::string& key,
                       std::vector<mojom::BlockchainTokenPtr> list);
  void UpdateChainList(ChainList chains);
  void UpdateRestrictedAddressesList(
      std::vector<std::string> restricted_addresses_list);
  void UpdatePolkadotRpcUrls(PolkadotRpcUrlsMap polkadot_rpc_urls);
  const PolkadotRpcUrlsMap& GetPolkadotRpcUrls() const;
  mojom::BlockchainTokenPtr GetTokenByAddress(const std::string& chain_id,
                                              mojom::CoinType coin,
                                              const std::string& address);
  std::vector<mojom::NetworkInfoPtr> GetPrepopulatedNetworks();
  std::optional<std::string> GetCoingeckoId(
      const std::string& chain_id,
      const std::string& contract_address);

  // BlockchainRegistry interface methods
  void GetTokenByAddress(const std::string& chain_id,
                         mojom::CoinType coin,
                         const std::string& address,
                         GetTokenByAddressCallback callback) override;
  void GetTokenBySymbol(const std::string& chain_id,
                        mojom::CoinType coin,
                        const std::string& symbol,
                        GetTokenBySymbolCallback callback) override;
  void GetAllTokens(const std::string& chain_id,
                    mojom::CoinType coin,
                    GetAllTokensCallback callback) override;
  TokenListMap GetEthTokenListMap(const std::vector<std::string>& chain_ids);
  void GetPrepopulatedNetworks(
      GetPrepopulatedNetworksCallback callback) override;
  void GetCoingeckoId(const std::string& chain_id,
                      const std::string& contract_address,
                      GetCoingeckoIdCallback callback) override;
  bool IsRestrictedAddress(const std::string& address);
  void ParseLists(const base::FilePath& dir, base::OnceClosure callback);

  class ScopedRestrictedAddressesForTesting {
   public:
    explicit ScopedRestrictedAddressesForTesting(
        std::vector<std::string> restricted_addresses);
    ~ScopedRestrictedAddressesForTesting();

    ScopedRestrictedAddressesForTesting(
        const ScopedRestrictedAddressesForTesting&) = delete;
    ScopedRestrictedAddressesForTesting& operator=(
        const ScopedRestrictedAddressesForTesting&) = delete;

   private:
    base::flat_set<std::string> previous_addresses_;
  };

  bool IsEmptyForTesting();
  void ResetForTesting();

 private:
  friend base::NoDestructor<BlockchainRegistry>;
  BlockchainRegistry();
  ~BlockchainRegistry() override;

  std::vector<mojom::BlockchainTokenPtr>* GetTokenListFromChainId(
      const std::string& chain_id);

  CoingeckoIdsMap coingecko_ids_map_;
  TokenListMap token_list_map_;
  ChainList chain_list_;
  base::flat_set<std::string> restricted_addresses_;
  PolkadotRpcUrlsMap polkadot_rpc_urls_;

  SEQUENCE_CHECKER(sequence_checker_);
  mojo::ReceiverSet<mojom::BlockchainRegistry> receivers_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_REGISTRY_H_
