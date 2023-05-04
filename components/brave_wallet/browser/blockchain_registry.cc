/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_registry.h"

#include <utility>

#include "base/containers/flat_set.h"
#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "net/base/url_util.h"

namespace brave_wallet {

BlockchainRegistry::BlockchainRegistry() = default;
BlockchainRegistry::~BlockchainRegistry() = default;

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

void BlockchainRegistry::UpdateTokenList(
    const std::string key,
    std::vector<mojom::BlockchainTokenPtr> list) {
  token_list_map_[key] = std::move(list);
}

void BlockchainRegistry::UpdateChainList(ChainList chains) {
  chain_list_ = std::move(chains);
}

void BlockchainRegistry::UpdateDappList(DappListMap dapp_lists) {
  dapp_lists_ = std::move(dapp_lists);
}

void BlockchainRegistry::GetTokenByAddress(const std::string& chain_id,
                                           mojom::CoinType coin,
                                           const std::string& address,
                                           GetTokenByAddressCallback callback) {
  std::move(callback).Run(GetTokenByAddress(chain_id, coin, address));
}

mojom::BlockchainTokenPtr BlockchainRegistry::GetTokenByAddress(
    const std::string& chain_id,
    mojom::CoinType coin,
    const std::string& address) {
  const auto key = GetTokenListKey(coin, chain_id);
  if (!token_list_map_.contains(key)) {
    return nullptr;
  }

  const auto& tokens = token_list_map_[key];
  auto token_it = base::ranges::find_if(
      tokens, [&](const mojom::BlockchainTokenPtr& current_token) {
        return current_token->contract_address == address;
      });
  return token_it == tokens.end() ? nullptr : token_it->Clone();
}

void BlockchainRegistry::GetTokenBySymbol(const std::string& chain_id,
                                          mojom::CoinType coin,
                                          const std::string& symbol,
                                          GetTokenBySymbolCallback callback) {
  const auto key = GetTokenListKey(coin, chain_id);
  if (!token_list_map_.contains(key)) {
    std::move(callback).Run(nullptr);
    return;
  }
  const auto& tokens = token_list_map_[key];
  auto token_it = base::ranges::find_if(
      tokens, [&](const mojom::BlockchainTokenPtr& current_token) {
        return current_token->symbol == symbol;
      });

  if (token_it == tokens.end()) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(token_it->Clone());
}

void BlockchainRegistry::GetAllTokens(const std::string& chain_id,
                                      mojom::CoinType coin,
                                      GetAllTokensCallback callback) {
  const auto key = GetTokenListKey(coin, chain_id);
  if (!token_list_map_.contains(key)) {
    std::move(callback).Run(std::vector<mojom::BlockchainTokenPtr>());
    return;
  }
  const auto& tokens = token_list_map_[key];
  std::vector<mojom::BlockchainTokenPtr> tokens_copy(tokens.size());
  std::transform(
      tokens.begin(), tokens.end(), tokens_copy.begin(),
      [](const mojom::BlockchainTokenPtr& current_token)
          -> mojom::BlockchainTokenPtr { return current_token.Clone(); });
  std::move(callback).Run(std::move(tokens_copy));
}

std::vector<mojom::BlockchainTokenPtr> BlockchainRegistry::GetBuyTokens(
    const std::vector<mojom::OnRampProvider>& providers,
    const std::string& chain_id) {
  std::vector<mojom::BlockchainTokenPtr> blockchain_buy_tokens;
  base::flat_set<mojom::OnRampProvider> provider_set(providers.begin(),
                                                     providers.end());

  for (const auto& provider : provider_set) {
    const std::vector<mojom::BlockchainToken>* buy_tokens = nullptr;
    if (provider == mojom::OnRampProvider::kRamp) {
      buy_tokens = &GetRampBuyTokens();
    } else if (provider == mojom::OnRampProvider::kSardine) {
      buy_tokens = &GetSardineBuyTokens();
    } else if (provider == mojom::OnRampProvider::kTransak) {
      buy_tokens = &GetTransakBuyTokens();
    } else {
      continue;
    }

    for (const auto& token : *buy_tokens) {
      if (token.chain_id == chain_id) {
        blockchain_buy_tokens.push_back(mojom::BlockchainToken::New(token));
      }
    }
  }

  return blockchain_buy_tokens;
}

TokenListMap BlockchainRegistry::GetEthTokenListMap(
    const std::vector<std::string>& chain_ids) {
  // Create a copy of token_list_map with only the chain_ids we want
  TokenListMap token_list_map_copy;
  for (const auto& chain_id : chain_ids) {
    const auto key = GetTokenListKey(mojom::CoinType::ETH, chain_id);
    // Skip if the key is not in the map.
    if (!token_list_map_.contains(key)) {
      continue;
    }

    // Otherwise, clone the vector of tokens.
    const auto& tokens = token_list_map_[key];
    std::vector<brave_wallet::mojom::BlockchainTokenPtr> tokens_copy(
        tokens.size());
    std::transform(
        tokens.begin(), tokens.end(), tokens_copy.begin(),
        [](const brave_wallet::mojom::BlockchainTokenPtr& current_token)
            -> brave_wallet::mojom::BlockchainTokenPtr {
          return current_token.Clone();
        });
    token_list_map_copy[chain_id] = std::move(tokens_copy);
  }

  return token_list_map_copy;
}

void BlockchainRegistry::GetBuyTokens(mojom::OnRampProvider provider,
                                      const std::string& chain_id,
                                      GetBuyTokensCallback callback) {
  std::move(callback).Run(GetBuyTokens({provider}, chain_id));
}

void BlockchainRegistry::GetProvidersBuyTokens(
    const std::vector<mojom::OnRampProvider>& providers,
    const std::string& chain_id,
    GetProvidersBuyTokensCallback callback) {
  std::move(callback).Run(GetBuyTokens(providers, chain_id));
}

void BlockchainRegistry::GetSellTokens(mojom::OffRampProvider provider,
                                       const std::string& chain_id,
                                       GetSellTokensCallback callback) {
  std::vector<mojom::BlockchainTokenPtr> blockchain_sell_tokens;
  const std::vector<mojom::BlockchainToken>* sell_tokens = nullptr;
  if (provider == mojom::OffRampProvider::kRamp) {
    sell_tokens = &GetRampSellTokens();
  }

  if (sell_tokens == nullptr) {
    std::move(callback).Run(std::move(blockchain_sell_tokens));
    return;
  }

  for (const auto& token : *sell_tokens) {
    if (token.chain_id != chain_id) {
      continue;
    }

    blockchain_sell_tokens.push_back(mojom::BlockchainToken::New(token));
  }
  std::move(callback).Run(std::move(blockchain_sell_tokens));
}

void BlockchainRegistry::GetOnRampCurrencies(
    GetOnRampCurrenciesCallback callback) {
  std::vector<mojom::OnRampCurrencyPtr> currencies;
  const std::vector<mojom::OnRampCurrency>* onRampCurrencies =
      &GetOnRampCurrenciesList();

  for (const auto& currency : *onRampCurrencies) {
    currencies.push_back(mojom::OnRampCurrency::New(currency));
  }
  std::move(callback).Run(std::move(currencies));
}

std::vector<mojom::NetworkInfoPtr>
BlockchainRegistry::GetPrepopulatedNetworks() {
  std::vector<mojom::NetworkInfoPtr> result;
  for (auto& chain : chain_list_) {
    if (auto known_chain =
            GetKnownChain(nullptr, chain->chain_id, mojom::CoinType::ETH)) {
      result.push_back(known_chain.Clone());
    } else {
      result.push_back(chain.Clone());
    }
  }
  return result;
}

void BlockchainRegistry::GetPrepopulatedNetworks(
    GetPrepopulatedNetworksCallback callback) {
  std::move(callback).Run(GetPrepopulatedNetworks());
}

}  // namespace brave_wallet
