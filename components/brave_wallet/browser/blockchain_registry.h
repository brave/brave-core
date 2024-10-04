/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_REGISTRY_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/sequence_checker.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace base {
template <typename T>
class NoDestructor;
class FilePath;
}  // namespace base

namespace brave_wallet {

class BlockchainRegistry : public mojom::BlockchainRegistry {
 public:
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
  void UpdateDappList(DappListMap dapp_lists);
  void UpdateOnRampTokenLists(OnRampTokensListMap onramp_lists);
  void UpdateOffRampTokenLists(OffRampTokensListMap onramp_lists);
  void UpdateOnRampCurrenciesLists(
      std::vector<mojom::OnRampCurrency> onramp_currencies_lists);
  void UpdateOfacAddressesList(std::vector<std::string> ofac_addresses_list);
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
  void GetBuyTokens(mojom::OnRampProvider provider,
                    const std::string& chain_id,
                    GetBuyTokensCallback callback) override;
  void GetProvidersBuyTokens(
      const std::vector<mojom::OnRampProvider>& providers,
      const std::string& chain_id,
      GetProvidersBuyTokensCallback callback) override;
  void GetSellTokens(mojom::OffRampProvider provider,
                     const std::string& chain_id,
                     GetSellTokensCallback callback) override;
  void GetOnRampCurrencies(GetOnRampCurrenciesCallback callback) override;
  void GetPrepopulatedNetworks(
      GetPrepopulatedNetworksCallback callback) override;
  void GetTopDapps(const std::string& chain_id,
                   mojom::CoinType coin,
                   GetTopDappsCallback callback) override;
  void GetCoingeckoId(const std::string& chain_id,
                      const std::string& contract_address,
                      GetCoingeckoIdCallback callback) override;
  bool IsOfacAddress(const std::string& address);
  void ParseLists(const base::FilePath& dir, base::OnceClosure callback);

  bool IsEmptyForTesting();
  void ResetForTesting();

 private:
  friend base::NoDestructor<BlockchainRegistry>;
  BlockchainRegistry();
  ~BlockchainRegistry() override;

  std::vector<mojom::BlockchainTokenPtr>* GetTokenListFromChainId(
      const std::string& chain_id);
  std::vector<brave_wallet::mojom::BlockchainTokenPtr> GetBuyTokens(
      const std::vector<mojom::OnRampProvider>& providers,
      const std::string& chain_id);

  CoingeckoIdsMap coingecko_ids_map_;
  TokenListMap token_list_map_;
  ChainList chain_list_;
  DappListMap dapp_lists_;
  OnRampTokensListMap on_ramp_token_lists_;
  OffRampTokensListMap off_ramp_token_lists_;
  std::vector<mojom::OnRampCurrency> on_ramp_currencies_list_;
  base::flat_set<std::string> ofac_addresses_;

  SEQUENCE_CHECKER(sequence_checker_);
  mojo::ReceiverSet<mojom::BlockchainRegistry> receivers_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_REGISTRY_H_
