/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_REGISTRY_H_

#include <string>
#include <vector>

#include "base/memory/singleton.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

class BlockchainRegistry : public mojom::BlockchainRegistry {
 public:
  BlockchainRegistry(const BlockchainRegistry&) = delete;
  ~BlockchainRegistry() override;
  BlockchainRegistry& operator=(const BlockchainRegistry&) = delete;

  static BlockchainRegistry* GetInstance();
  mojo::PendingRemote<mojom::BlockchainRegistry> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BlockchainRegistry> receiver);

  void UpdateTokenList(TokenListMap tokens);
  void UpdateTokenList(const std::string key,
                       std::vector<mojom::BlockchainTokenPtr> list);
  void UpdateChainList(ChainList chains);

  mojom::BlockchainTokenPtr GetTokenByAddress(const std::string& chain_id,
                                              mojom::CoinType coin,
                                              const std::string& address);
  std::vector<mojom::NetworkInfoPtr> GetPrepopulatedNetworks();

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
  void GetBuyTokens(mojom::OnRampProvider provider,
                    const std::string& chain_id,
                    GetBuyTokensCallback callback) override;
  void GetBuyUrl(mojom::OnRampProvider provider,
                 const std::string& chain_id,
                 const std::string& address,
                 const std::string& symbol,
                 const std::string& amount,
                 GetBuyUrlCallback callback) override;
  void GetOnRampCurrencies(GetOnRampCurrenciesCallback callback) override;
  void GetPrepopulatedNetworks(
      GetPrepopulatedNetworksCallback callback) override;

 protected:
  std::vector<mojom::BlockchainTokenPtr>* GetTokenListFromChainId(
      const std::string& chain_id);

  TokenListMap token_list_map_;
  ChainList chain_list_;
  friend struct base::DefaultSingletonTraits<BlockchainRegistry>;

  BlockchainRegistry();

 private:
  mojo::ReceiverSet<mojom::BlockchainRegistry> receivers_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_REGISTRY_H_
