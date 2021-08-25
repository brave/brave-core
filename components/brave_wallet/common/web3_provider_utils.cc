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

void ValueToEthereumChain(const base::Value& value,
                          mojom::EthereumChain* chain) {
  if (!chain)
    return;
  const base::DictionaryValue* params_dict;
  value.GetAsDictionary(&params_dict);
  if (!params_dict)
    return;

  if (!params_dict->GetString("chainId", &chain->chain_id))
    return;

  params_dict->GetString("chainName", &chain->chain_name);

  const base::ListValue* explorerUrlsList;
  if (params_dict->GetList("blockExplorerUrls", &explorerUrlsList)) {
    for (const auto& entry : explorerUrlsList->GetList())
      chain->block_explorer_urls =
          std::vector<std::string>({entry.GetString()});
  }

  const base::ListValue* iconUrlsList;
  if (params_dict->GetList("iconUrls", &iconUrlsList)) {
    for (const auto& entry : iconUrlsList->GetList())
      chain->icon_urls = std::vector<std::string>({entry.GetString()});
  }

  const base::ListValue* rpcUrlsList;
  if (params_dict->GetList("rpcUrls", &rpcUrlsList)) {
    for (const auto& entry : rpcUrlsList->GetList())
      chain->rpc_urls.push_back(entry.GetString());
  }
  const base::DictionaryValue* currency_dict;
  if (params_dict->GetDictionary("nativeCurrency", &currency_dict)) {
    currency_dict->GetString("name", &chain->name);
    currency_dict->GetString("symbol", &chain->symbol);
    chain->decimals =
        currency_dict->FindIntPath("decimals").value_or(0);
  }
}

}  // namespace brave_wallet
