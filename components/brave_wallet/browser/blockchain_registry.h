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

  mojom::BlockchainTokenPtr GetTokenByContract(const std::string& chain_id,
                                               const std::string& contract);

  // BlockchainRegistry interface methods
  void GetTokenByContract(const std::string& chain_id,
                          const std::string& contract,
                          GetTokenByContractCallback callback) override;
  void GetTokenBySymbol(const std::string& chain_id,
                        const std::string& symbol,
                        GetTokenBySymbolCallback callback) override;
  void GetAllTokens(const std::string& chain_id,
                    GetAllTokensCallback callback) override;
  void GetBuyTokens(const std::string& chain_id,
                    GetBuyTokensCallback callback) override;
  void GetBuyUrl(const std::string& chain_id,
                 const std::string& address,
                 const std::string& symbol,
                 const std::string& amount,
                 GetBuyUrlCallback callback) override;

 protected:
  std::vector<mojom::BlockchainTokenPtr>* GetTokenListFromChainId(
      const std::string& chain_id);

  TokenListMap token_list_map_;
  friend struct base::DefaultSingletonTraits<BlockchainRegistry>;

  BlockchainRegistry();

 private:
  mojo::ReceiverSet<mojom::BlockchainRegistry> receivers_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BLOCKCHAIN_REGISTRY_H_
