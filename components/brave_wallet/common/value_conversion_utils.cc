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

namespace brave_wallet {

mojom::NetworkInfoPtr ValueToEthNetworkInfo(const base::Value& value) {
  mojom::NetworkInfo chain;
  const base::DictionaryValue* params_dict = nullptr;
  if (!value.GetAsDictionary(&params_dict) || !params_dict)
    return nullptr;

  const std::string* chain_id = params_dict->FindStringKey("chainId");
  if (!chain_id) {
    return nullptr;
  }
  chain.chain_id = *chain_id;

  const std::string* chain_name = params_dict->FindStringKey("chainName");
  if (chain_name) {
    chain.chain_name = *chain_name;
  }

  const base::Value* explorerUrlsListValue =
      params_dict->FindListKey("blockExplorerUrls");
  if (explorerUrlsListValue) {
    for (const auto& entry : explorerUrlsListValue->GetList())
      chain.block_explorer_urls.push_back(entry.GetString());
  }

  const base::Value* iconUrlsValue = params_dict->FindListKey("iconUrls");
  if (iconUrlsValue) {
    for (const auto& entry : iconUrlsValue->GetList())
      chain.icon_urls.push_back(entry.GetString());
  }

  const base::Value* rpcUrlsValue = params_dict->FindListKey("rpcUrls");
  if (rpcUrlsValue) {
    for (const auto& entry : rpcUrlsValue->GetList())
      chain.rpc_urls.push_back(entry.GetString());
  }
  const base::Value* nativeCurrencyValue =
      params_dict->FindDictKey("nativeCurrency");
  chain.decimals = 0;
  if (nativeCurrencyValue) {
    const std::string* symbol_name = nativeCurrencyValue->FindStringKey("name");
    if (symbol_name) {
      chain.symbol_name = *symbol_name;
    }
    const std::string* symbol = nativeCurrencyValue->FindStringKey("symbol");
    if (symbol) {
      chain.symbol = *symbol;
    }
    absl::optional<int> decimals = nativeCurrencyValue->FindIntKey("decimals");
    if (decimals) {
      chain.decimals = decimals.value();
    }
  }

  chain.coin = mojom::CoinType::ETH;

  absl::optional<bool> is_eip1559 = params_dict->FindBoolKey("is_eip1559");
  if (is_eip1559) {
    chain.data = mojom::NetworkInfoData::NewEthData(
        mojom::NetworkInfoDataETH::New(*is_eip1559));
  }

  return chain.Clone();
}

base::Value EthNetworkInfoToValue(const mojom::NetworkInfoPtr& chain) {
  base::Value dict(base::Value::Type::DICTIONARY);
  DCHECK_EQ(chain->coin, mojom::CoinType::ETH);
  dict.SetStringKey("chainId", chain->chain_id);
  dict.SetStringKey("chainName", chain->chain_name);
  bool is_eip1559 = false;
  if (chain->data && chain->data->is_eth_data()) {
    is_eip1559 = chain->data->get_eth_data()->is_eip1559;
  }
  dict.SetBoolKey("is_eip1559", is_eip1559);

  base::ListValue blockExplorerUrlsValue;
  if (!chain->block_explorer_urls.empty()) {
    for (const auto& url : chain->block_explorer_urls) {
      blockExplorerUrlsValue.Append(url);
    }
  }
  dict.SetKey("blockExplorerUrls", std::move(blockExplorerUrlsValue));

  base::ListValue iconUrlsValue;
  if (!chain->icon_urls.empty()) {
    for (const auto& url : chain->icon_urls) {
      iconUrlsValue.Append(url);
    }
  }
  dict.SetKey("iconUrls", std::move(iconUrlsValue));

  base::ListValue rpcUrlsValue;
  for (const auto& url : chain->rpc_urls) {
    rpcUrlsValue.Append(url);
  }
  dict.SetKey("rpcUrls", std::move(rpcUrlsValue));
  base::Value currency(base::Value::Type::DICTIONARY);
  currency.SetStringKey("name", chain->symbol_name);
  currency.SetStringKey("symbol", chain->symbol);
  currency.SetIntKey("decimals", chain->decimals);
  dict.SetKey("nativeCurrency", std::move(currency));
  return dict;
}

mojom::BlockchainTokenPtr ValueToBlockchainToken(const base::Value& value) {
  mojom::BlockchainTokenPtr tokenPtr = mojom::BlockchainToken::New();
  if (!value.is_dict())
    return nullptr;

  const std::string* contract_address = value.FindStringKey("contract_address");
  if (!contract_address)
    return nullptr;
  tokenPtr->contract_address = *contract_address;

  const std::string* name = value.FindStringKey("name");
  if (!name)
    return nullptr;
  tokenPtr->name = *name;

  const std::string* symbol = value.FindStringKey("symbol");
  if (!symbol)
    return nullptr;
  tokenPtr->symbol = *symbol;

  const std::string* logo = value.FindStringKey("logo");
  if (logo) {
    tokenPtr->logo = *logo;
  }

  absl::optional<bool> is_erc20 = value.FindBoolKey("is_erc20");
  if (!is_erc20)
    return nullptr;
  tokenPtr->is_erc20 = is_erc20.value();

  absl::optional<bool> is_erc721 = value.FindBoolKey("is_erc721");
  if (!is_erc721)
    return nullptr;
  tokenPtr->is_erc721 = is_erc721.value();

  absl::optional<int> decimals = value.FindIntKey("decimals");
  if (!decimals)
    return nullptr;
  tokenPtr->decimals = decimals.value();

  absl::optional<bool> visible = value.FindBoolKey("visible");
  if (!visible)
    return nullptr;
  tokenPtr->visible = visible.value();

  const std::string* token_id = value.FindStringKey("token_id");
  if (token_id)
    tokenPtr->token_id = *token_id;

  const std::string* coingecko_id = value.FindStringKey("coingecko_id");
  if (coingecko_id)
    tokenPtr->coingecko_id = *coingecko_id;

  return tokenPtr;
}

// Creates a response object as described in:
// https://eips.ethereum.org/EIPS/eip-2255
base::ListValue PermissionRequestResponseToValue(
    const url::Origin& origin,
    const std::vector<std::string> accounts) {
  base::ListValue container_list;
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("id", base::GenerateGUID());

  base::ListValue context_list;
  context_list.Append(base::Value("https://github.com/MetaMask/rpc-cap"));
  dict.SetKey("context", std::move(context_list));

  base::ListValue caveats_list;
  base::Value caveats_obj1(base::Value::Type::DICTIONARY);
  caveats_obj1.SetStringKey("name", "primaryAccountOnly");
  caveats_obj1.SetStringKey("type", "limitResponseLength");
  caveats_obj1.SetIntKey("value", 1);
  caveats_list.Append(std::move(caveats_obj1));
  base::Value caveats_obj2(base::Value::Type::DICTIONARY);
  caveats_obj2.SetStringKey("name", "exposedAccounts");
  caveats_obj2.SetStringKey("type", "filterResponse");
  base::ListValue filter_response_list;
  for (auto account : accounts) {
    filter_response_list.Append(base::Value(account));
  }
  caveats_obj2.SetKey("value", std::move(filter_response_list));
  caveats_list.Append(std::move(caveats_obj2));
  dict.SetKey("caveats", std::move(caveats_list));

  dict.SetDoubleKey("date", base::Time::Now().ToJsTime());
  dict.SetStringKey("invoker", origin.Serialize());
  dict.SetStringKey("parentCapability", "eth_accounts");
  container_list.Append(std::move(dict));
  return container_list;
}

}  // namespace brave_wallet
