/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_registry.h"

#include <utility>

#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
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
  if (!token_list_map_.contains(key))
    return nullptr;

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
    std::move(callback).Run(
        std::vector<brave_wallet::mojom::BlockchainTokenPtr>());
    return;
  }
  const auto& tokens = token_list_map_[key];
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

void BlockchainRegistry::GetBuyTokens(mojom::OnRampProvider provider,
                                      const std::string& chain_id,
                                      GetBuyTokensCallback callback) {
  std::vector<brave_wallet::mojom::BlockchainTokenPtr> blockchain_buy_tokens;
  const std::vector<mojom::BlockchainToken>* buy_tokens = nullptr;
  if (provider == mojom::OnRampProvider::kWyre)
    buy_tokens = &GetWyreBuyTokens();
  else if (provider == mojom::OnRampProvider::kRamp)
    buy_tokens = &GetRampBuyTokens();
  else if (provider == mojom::OnRampProvider::kSardine)
    buy_tokens = &GetSardineBuyTokens();

  if (buy_tokens == nullptr) {
    std::move(callback).Run(std::move(blockchain_buy_tokens));
    return;
  }

  for (const auto& token : *buy_tokens) {
    if (token.chain_id != chain_id) {
      continue;
    }

    blockchain_buy_tokens.push_back(
        brave_wallet::mojom::BlockchainToken::New(token));
  }
  std::move(callback).Run(std::move(blockchain_buy_tokens));
}
// TODO(muliswilliam) - Remove this function when iOS and Android no longer
// depend on it https://github.com/brave/brave-browser/issues/23503
void BlockchainRegistry::GetBuyUrl(mojom::OnRampProvider provider,
                                   const std::string& chain_id,
                                   const std::string& address,
                                   const std::string& symbol,
                                   const std::string& amount,
                                   GetBuyUrlCallback callback) {
  std::string url;
  const std::string default_currency = "USD";
  if (provider == mojom::OnRampProvider::kWyre) {
    GURL wyre_url = GURL(kWyreBaseUrl);
    wyre_url =
        net::AppendQueryParameter(wyre_url, "dest", "ethereum:" + address);
    wyre_url =
        net::AppendQueryParameter(wyre_url, "sourceCurrency", default_currency);
    wyre_url = net::AppendQueryParameter(wyre_url, "destCurrency", symbol);
    wyre_url = net::AppendQueryParameter(wyre_url, "amount", amount);
    wyre_url = net::AppendQueryParameter(wyre_url, "accountId", kWyreID);
    wyre_url =
        net::AppendQueryParameter(wyre_url, "paymentMethod", "debit-card");
    std::move(callback).Run(std::move(wyre_url.spec()), absl::nullopt);
  } else if (provider == mojom::OnRampProvider::kRamp) {
    GURL ramp_url = GURL(kRampBaseUrl);
    ramp_url = net::AppendQueryParameter(ramp_url, "userAddress", address);
    ramp_url = net::AppendQueryParameter(ramp_url, "swapAsset", symbol);
    ramp_url = net::AppendQueryParameter(ramp_url, "fiatValue", amount);
    ramp_url =
        net::AppendQueryParameter(ramp_url, "fiatCurrency", default_currency);
    ramp_url = net::AppendQueryParameter(ramp_url, "hostApiKey", kRampID);
    std::move(callback).Run(std::move(ramp_url.spec()), absl::nullopt);
  } else {
    std::move(callback).Run(url, "UNSUPPORTED_ONRAMP_PROVIDER");
  }
}

void BlockchainRegistry::GetOnRampCurrencies(
    GetOnRampCurrenciesCallback callback) {
  std::vector<brave_wallet::mojom::OnRampCurrencyPtr> currencies;
  const std::vector<mojom::OnRampCurrency>* onRampCurrencies =
      &GetOnRampCurrenciesList();

  for (const auto& currency : *onRampCurrencies) {
    currencies.push_back(brave_wallet::mojom::OnRampCurrency::New(currency));
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
