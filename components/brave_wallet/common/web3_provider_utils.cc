/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/web3_provider_utils.h"

namespace brave_wallet {

absl::optional<mojom::EthereumChain> ParameterValueToEthereumChain(
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
      chain.name = *symbol_name;
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

}  // namespace brave_wallet
