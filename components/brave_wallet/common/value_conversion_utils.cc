/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"

namespace brave_wallet {

absl::optional<mojom::EthereumChain> ValueToEthereumChain(
    const base::Value& value) {
  mojom::EthereumChain chain;
  const base::DictionaryValue* params_dict = nullptr;
  if (!value.GetAsDictionary(&params_dict) || !params_dict)
    return absl::nullopt;

  const std::string* chain_id = params_dict->FindStringKey("chainId");
  if (!chain_id) {
    return absl::nullopt;
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
  return chain;
}

base::Value EthereumChainToValue(const mojom::EthereumChainPtr& chain) {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("chainId", chain->chain_id);
  dict.SetStringKey("chainName", chain->chain_name);

  base::ListValue blockExplorerUrlsValue;
  if (!chain->block_explorer_urls.empty()) {
    for (const auto& url : chain->block_explorer_urls) {
      blockExplorerUrlsValue.AppendString(url);
    }
  }
  dict.SetKey("blockExplorerUrls", std::move(blockExplorerUrlsValue));

  base::ListValue iconUrlsValue;
  if (!chain->icon_urls.empty()) {
    for (const auto& url : chain->icon_urls) {
      iconUrlsValue.AppendString(url);
    }
  }
  dict.SetKey("iconUrls", std::move(iconUrlsValue));

  base::ListValue rpcUrlsValue;
  for (const auto& url : chain->rpc_urls) {
    rpcUrlsValue.AppendString(url);
  }
  dict.SetKey("rpcUrls", std::move(rpcUrlsValue));
  base::Value currency(base::Value::Type::DICTIONARY);
  currency.SetStringKey("name", chain->symbol_name);
  currency.SetStringKey("symbol", chain->symbol);
  currency.SetIntKey("decimals", chain->decimals);
  dict.SetKey("nativeCurrency", std::move(currency));
  return dict;
}

}  // namespace brave_wallet
