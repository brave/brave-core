/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_registry.h"

#include <algorithm>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"

namespace brave_wallet {

BlockchainRegistry::BlockchainRegistry() = default;

BlockchainRegistry::~BlockchainRegistry() {}

BlockchainRegistry* BlockchainRegistry::GetInstance() {
  return base::Singleton<BlockchainRegistry>::get();
}

mojo::PendingRemote<mojom::BlockchainRegistry>
BlockchainRegistry::MakeRemote() {
  mojo::PendingRemote<mojom::BlockchainRegistry> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BlockchainRegistry::Bind(
    mojo::PendingReceiver<mojom::BlockchainRegistry> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BlockchainRegistry::UpdateTokenList(TokenListMap token_list_map) {
  token_list_map_ = std::move(token_list_map);
}

void BlockchainRegistry::GetTokenByContract(
    const std::string& chain_id,
    const std::string& contract,
    GetTokenByContractCallback callback) {
  std::move(callback).Run(GetTokenByContract(chain_id, contract));
}

mojom::BlockchainTokenPtr BlockchainRegistry::GetTokenByContract(
    const std::string& chain_id,
    const std::string& contract) {
  if (token_list_map_.find(chain_id) == token_list_map_.end())
    return nullptr;

  const auto& tokens = token_list_map_[chain_id];
  auto token_it =
      std::find_if(tokens.begin(), tokens.end(),
                   [&](const mojom::BlockchainTokenPtr& current_token) {
                     return current_token->contract_address == contract;
                   });
  return token_it == tokens.end() ? nullptr : token_it->Clone();
}

void BlockchainRegistry::GetTokenBySymbol(const std::string& chain_id,
                                          const std::string& symbol,
                                          GetTokenBySymbolCallback callback) {
  if (token_list_map_.find(chain_id) == token_list_map_.end()) {
    std::move(callback).Run(nullptr);
    return;
  }
  const auto& tokens = token_list_map_[chain_id];
  auto token_it =
      std::find_if(tokens.begin(), tokens.end(),
                   [&](const mojom::BlockchainTokenPtr& current_token) {
                     return current_token->symbol == symbol;
                   });

  if (token_it == tokens.end()) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(token_it->Clone());
}

void BlockchainRegistry::GetAllTokens(const std::string& chain_id,
                                      GetAllTokensCallback callback) {
  if (token_list_map_.find(chain_id) == token_list_map_.end()) {
    std::move(callback).Run(
        std::vector<brave_wallet::mojom::BlockchainTokenPtr>());
    return;
  }
  const auto& tokens = token_list_map_[chain_id];
  std::vector<brave_wallet::mojom::BlockchainTokenPtr> tokens_copy(
      tokens.size());
  std::transform(
      tokens.begin(), tokens.end(), tokens_copy.begin(),
      [](const brave_wallet::mojom::BlockchainTokenPtr& current_token)
          -> brave_wallet::mojom::BlockchainTokenPtr {
        return current_token.Clone();
      });
  std::move(callback).Run(std::move(tokens_copy));
}

void BlockchainRegistry::GetBuyTokens(const std::string& chain_id,
                                      GetBuyTokensCallback callback) {
  std::vector<brave_wallet::mojom::BlockchainTokenPtr> blockchain_buy_tokens;
  for (auto token : *kBuyTokens) {
    auto blockchain_token = brave_wallet::mojom::BlockchainToken::New();
    *blockchain_token = token;
    blockchain_buy_tokens.push_back(std::move(blockchain_token));
  }
  std::move(callback).Run(std::move(blockchain_buy_tokens));
}

void BlockchainRegistry::GetBuyUrl(const std::string& chain_id,
                                   const std::string& address,
                                   const std::string& symbol,
                                   const std::string& amount,
                                   GetBuyUrlCallback callback) {
  std::string url = base::StringPrintf(kBuyUrl, address.c_str(), symbol.c_str(),
                                       amount.c_str(), kWyreID);

  std::move(callback).Run(url);
}

}  // namespace brave_wallet
