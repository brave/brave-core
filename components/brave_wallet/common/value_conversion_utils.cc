/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "net/base/url_util.h"

namespace {

// Allow only HTTPS or localhost HTTP URLs in params of AddEthereumChain dApp
// requests.
bool IsValidURL(const std::string& url_string) {
  GURL url(url_string);
  return url.is_valid() &&
         (url.SchemeIs(url::kHttpsScheme) ||
          (net::IsLocalhost(url) && url.SchemeIs(url::kHttpScheme)));
}

}  // namespace

namespace brave_wallet {

absl::optional<std::string> ExtractChainIdFromValue(
    const base::Value::Dict* dict) {
  if (!dict)
    return absl::nullopt;

  const std::string* chain_id = dict->FindString("chainId");
  if (!chain_id) {
    return absl::nullopt;
  }
  return *chain_id;
}

mojom::NetworkInfoPtr ValueToEthNetworkInfo(const base::Value& value,
                                            bool check_url) {
  mojom::NetworkInfo chain;
  const base::Value::Dict* params_dict = value.GetIfDict();
  if (!params_dict)
    return nullptr;

  const std::string* chain_id = params_dict->FindString("chainId");
  if (!chain_id) {
    return nullptr;
  }
  chain.chain_id = *chain_id;

  const std::string* chain_name = params_dict->FindString("chainName");
  if (chain_name) {
    chain.chain_name = *chain_name;
  }

  const auto* explorerUrlsListValue =
      params_dict->FindList("blockExplorerUrls");
  if (explorerUrlsListValue) {
    for (const auto& entry : *explorerUrlsListValue) {
      if (!entry.is_string())
        continue;
      if (!check_url || IsValidURL(entry.GetString()))
        chain.block_explorer_urls.push_back(entry.GetString());
    }
  }

  const auto* iconUrlsValue = params_dict->FindList("iconUrls");
  if (iconUrlsValue) {
    for (const auto& entry : *iconUrlsValue) {
      if (!entry.is_string())
        continue;
      if (!check_url || IsValidURL(entry.GetString()))
        chain.icon_urls.push_back(entry.GetString());
    }
  }

  const auto* rpcUrlsValue = params_dict->FindList("rpcUrls");
  if (rpcUrlsValue) {
    for (const auto& entry : *rpcUrlsValue) {
      if (!entry.is_string())
        continue;
      if (!check_url || IsValidURL(entry.GetString()))
        chain.rpc_urls.push_back(entry.GetString());
    }
  }
  const auto* nativeCurrencyValue = params_dict->FindDict("nativeCurrency");
  chain.decimals = 0;
  if (nativeCurrencyValue) {
    const std::string* symbol_name = nativeCurrencyValue->FindString("name");
    if (symbol_name) {
      chain.symbol_name = *symbol_name;
    }
    const std::string* symbol = nativeCurrencyValue->FindString("symbol");
    if (symbol) {
      chain.symbol = *symbol;
    }
    absl::optional<int> decimals = nativeCurrencyValue->FindInt("decimals");
    if (decimals) {
      chain.decimals = decimals.value();
    }
  }

  chain.coin = mojom::CoinType::ETH;

  chain.is_eip1559 = params_dict->FindBool("is_eip1559").value_or(false);

  return chain.Clone();
}

base::Value::Dict EthNetworkInfoToValue(const mojom::NetworkInfo& chain) {
  base::Value::Dict dict;
  DCHECK_EQ(chain.coin, mojom::CoinType::ETH);
  dict.Set("chainId", chain.chain_id);
  dict.Set("chainName", chain.chain_name);
  dict.Set("is_eip1559", chain.is_eip1559);

  base::Value::List blockExplorerUrlsValue;
  if (!chain.block_explorer_urls.empty()) {
    for (const auto& url : chain.block_explorer_urls) {
      blockExplorerUrlsValue.Append(url);
    }
  }
  dict.Set("blockExplorerUrls", std::move(blockExplorerUrlsValue));

  base::Value::List iconUrlsValue;
  if (!chain.icon_urls.empty()) {
    for (const auto& url : chain.icon_urls) {
      iconUrlsValue.Append(url);
    }
  }
  dict.Set("iconUrls", std::move(iconUrlsValue));

  base::Value::List rpcUrlsValue;
  for (const auto& url : chain.rpc_urls) {
    rpcUrlsValue.Append(url);
  }
  dict.Set("rpcUrls", std::move(rpcUrlsValue));
  base::Value::Dict currency;
  currency.Set("name", chain.symbol_name);
  currency.Set("symbol", chain.symbol);
  currency.Set("decimals", chain.decimals);
  dict.Set("nativeCurrency", std::move(currency));
  return dict;
}

mojom::BlockchainTokenPtr ValueToBlockchainToken(const base::Value::Dict& value,
                                                 const std::string& chain_id,
                                                 mojom::CoinType coin) {
  mojom::BlockchainTokenPtr tokenPtr = mojom::BlockchainToken::New();

  const std::string* contract_address = value.FindString("address");
  if (!contract_address)
    return nullptr;
  tokenPtr->contract_address = *contract_address;

  const std::string* name = value.FindString("name");
  if (!name)
    return nullptr;
  tokenPtr->name = *name;

  const std::string* symbol = value.FindString("symbol");
  if (!symbol)
    return nullptr;
  tokenPtr->symbol = *symbol;

  const std::string* logo = value.FindString("logo");
  if (logo) {
    tokenPtr->logo = *logo;
  }

  absl::optional<bool> is_erc20 = value.FindBool("is_erc20");
  if (!is_erc20)
    return nullptr;
  tokenPtr->is_erc20 = is_erc20.value();

  absl::optional<bool> is_erc721 = value.FindBool("is_erc721");
  if (!is_erc721)
    return nullptr;
  tokenPtr->is_erc721 = is_erc721.value();

  absl::optional<int> decimals = value.FindInt("decimals");
  if (!decimals)
    return nullptr;
  tokenPtr->decimals = decimals.value();

  absl::optional<bool> visible = value.FindBool("visible");
  if (!visible)
    return nullptr;
  tokenPtr->visible = visible.value();

  const std::string* token_id = value.FindString("token_id");
  if (token_id)
    tokenPtr->token_id = *token_id;

  const std::string* coingecko_id = value.FindString("coingecko_id");
  if (coingecko_id)
    tokenPtr->coingecko_id = *coingecko_id;

  tokenPtr->coin = coin;
  tokenPtr->chain_id = chain_id;

  return tokenPtr;
}

// Creates a response object as described in:
// https://eips.ethereum.org/EIPS/eip-2255
base::Value::List PermissionRequestResponseToValue(
    const url::Origin& origin,
    const std::vector<std::string> accounts) {
  base::Value::List container_list;
  base::Value::Dict dict;
  dict.Set("id", base::GenerateGUID());

  base::Value::List context_list;
  context_list.Append(base::Value("https://github.com/MetaMask/rpc-cap"));
  dict.Set("context", std::move(context_list));

  base::Value::List caveats_list;
  base::Value::Dict caveats_obj1;
  caveats_obj1.Set("name", "primaryAccountOnly");
  caveats_obj1.Set("type", "limitResponseLength");
  caveats_obj1.Set("value", 1);
  caveats_list.Append(std::move(caveats_obj1));
  base::Value::Dict caveats_obj2;
  caveats_obj2.Set("name", "exposedAccounts");
  caveats_obj2.Set("type", "filterResponse");
  base::Value::List filter_response_list;
  for (auto account : accounts) {
    filter_response_list.Append(account);
  }
  caveats_obj2.Set("value", std::move(filter_response_list));
  caveats_list.Append(std::move(caveats_obj2));
  dict.Set("caveats", std::move(caveats_list));

  dict.Set("date", base::Time::Now().ToJsTime());
  dict.Set("invoker", origin.Serialize());
  dict.Set("parentCapability", "eth_accounts");
  container_list.Append(std::move(dict));
  return container_list;
}

}  // namespace brave_wallet
