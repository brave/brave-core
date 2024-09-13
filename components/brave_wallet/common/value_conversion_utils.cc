/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/value_conversion_utils.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/net/base/url_util.h"

namespace {

// Common parts of base::Value parsing shared between Eip3085 payload spec and
// brave settings pserstence.
// IMPORTANT: When adding something here please make sure it is valid for
// https://eips.ethereum.org/EIPS/eip-3085.
bool ValueToNetworkInfoCommon(const base::Value& value,
                              brave_wallet::mojom::NetworkInfo* chain) {
  const base::Value::Dict* params_dict = value.GetIfDict();
  if (!params_dict) {
    return false;
  }

  const std::string* chain_id = params_dict->FindString("chainId");
  if (!chain_id) {
    return false;
  }
  chain->chain_id = *chain_id;

  const std::string* chain_name = params_dict->FindString("chainName");
  if (chain_name) {
    chain->chain_name = *chain_name;
  }

  const auto* nativeCurrencyValue = params_dict->FindDict("nativeCurrency");
  chain->decimals = 0;
  if (nativeCurrencyValue) {
    const std::string* symbol_name = nativeCurrencyValue->FindString("name");
    if (symbol_name) {
      chain->symbol_name = *symbol_name;
    }
    const std::string* symbol = nativeCurrencyValue->FindString("symbol");
    if (symbol) {
      chain->symbol = *symbol;
    }
    std::optional<int> decimals = nativeCurrencyValue->FindInt("decimals");
    if (decimals) {
      chain->decimals = decimals.value();
    }
  }

  return true;
}

}  // namespace

namespace brave_wallet {

std::optional<std::string> ExtractChainIdFromValue(
    const base::Value::Dict* dict) {
  if (!dict) {
    return std::nullopt;
  }

  const std::string* chain_id = dict->FindString("chainId");
  if (!chain_id) {
    return std::nullopt;
  }
  return *chain_id;
}

mojom::NetworkInfoPtr ValueToNetworkInfo(const base::Value& value) {
  mojom::NetworkInfo chain;

  if (!ValueToNetworkInfoCommon(value, &chain)) {
    return nullptr;
  }

  const base::Value::Dict* params_dict = value.GetIfDict();
  if (!params_dict) {
    return nullptr;
  }

  if (const auto coin = params_dict->FindInt("coin")) {
    chain.coin = static_cast<mojom::CoinType>(*coin);
  } else {
    chain.coin = mojom::CoinType::ETH;
  }

  chain.supported_keyrings =
      GetSupportedKeyringsForNetwork(chain.coin, chain.chain_id);

  const auto* explorerUrlsListValue =
      params_dict->FindList("blockExplorerUrls");
  if (explorerUrlsListValue) {
    for (const auto& entry : *explorerUrlsListValue) {
      if (!entry.is_string()) {
        continue;
      }
      chain.block_explorer_urls.push_back(entry.GetString());
    }
  }

  const auto* iconUrlsValue = params_dict->FindList("iconUrls");
  if (iconUrlsValue) {
    for (const auto& entry : *iconUrlsValue) {
      if (!entry.is_string()) {
        continue;
      }
      chain.icon_urls.push_back(entry.GetString());
    }
  }

  const auto* rpcUrlsValue = params_dict->FindList("rpcUrls");
  if (rpcUrlsValue) {
    for (const auto& entry : *rpcUrlsValue) {
      if (!entry.is_string()) {
        continue;
      }
      chain.rpc_endpoints.emplace_back(entry.GetString());
    }
  }

  if (const auto& endpoint_index =
          params_dict->FindInt("activeRpcEndpointIndex")) {
    chain.active_rpc_endpoint_index = endpoint_index.value();
    if (chain.active_rpc_endpoint_index < 0 ||
        static_cast<size_t>(chain.active_rpc_endpoint_index) >
            chain.rpc_endpoints.size()) {
      chain.active_rpc_endpoint_index = 0;
    }
  } else {
    chain.active_rpc_endpoint_index =
        GetFirstValidChainURLIndex(chain.rpc_endpoints);
  }

  return chain.Clone();
}

mojom::NetworkInfoPtr ParseEip3085Payload(const base::Value& value) {
  mojom::NetworkInfo chain;

  if (!ValueToNetworkInfoCommon(value, &chain)) {
    return nullptr;
  }

  chain.coin = mojom::CoinType::ETH;

  const base::Value::Dict* params_dict = value.GetIfDict();
  if (!params_dict) {
    return nullptr;
  }

  const auto* explorerUrlsListValue =
      params_dict->FindList("blockExplorerUrls");
  if (explorerUrlsListValue) {
    for (const auto& entry : *explorerUrlsListValue) {
      if (!entry.is_string() || !IsHTTPSOrLocalhostURL(entry.GetString())) {
        continue;
      }
      chain.block_explorer_urls.push_back(entry.GetString());
    }
  }

  const auto* iconUrlsValue = params_dict->FindList("iconUrls");
  if (iconUrlsValue) {
    for (const auto& entry : *iconUrlsValue) {
      if (!entry.is_string() || !IsHTTPSOrLocalhostURL(entry.GetString())) {
        continue;
      }
      chain.icon_urls.push_back(entry.GetString());
    }
  }

  const auto* rpcUrlsValue = params_dict->FindList("rpcUrls");
  if (rpcUrlsValue) {
    for (const auto& entry : *rpcUrlsValue) {
      if (!entry.is_string() || !IsHTTPSOrLocalhostURL(entry.GetString())) {
        continue;
      }
      chain.rpc_endpoints.emplace_back(entry.GetString());
    }
  }

  chain.active_rpc_endpoint_index =
      GetFirstValidChainURLIndex(chain.rpc_endpoints);

  return chain.Clone();
}

base::Value::Dict NetworkInfoToValue(const mojom::NetworkInfo& chain) {
  base::Value::Dict dict;

  dict.Set("coin", static_cast<int>(chain.coin));
  dict.Set("chainId", chain.chain_id);
  dict.Set("chainName", chain.chain_name);

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
  for (const auto& url : chain.rpc_endpoints) {
    rpcUrlsValue.Append(url.spec());
  }
  dict.Set("rpcUrls", std::move(rpcUrlsValue));
  DCHECK_GE(chain.active_rpc_endpoint_index, 0);
  DCHECK_LT(static_cast<size_t>(chain.active_rpc_endpoint_index),
            chain.rpc_endpoints.size());
  dict.Set("activeRpcEndpointIndex", chain.active_rpc_endpoint_index);
  base::Value::Dict currency;
  currency.Set("name", chain.symbol_name);
  currency.Set("symbol", chain.symbol);
  currency.Set("decimals", chain.decimals);
  dict.Set("nativeCurrency", std::move(currency));
  return dict;
}

mojom::BlockchainTokenPtr ValueToBlockchainToken(
    const base::Value::Dict& value) {
  mojom::BlockchainTokenPtr token_ptr = mojom::BlockchainToken::New();

  auto coin_int = value.FindInt("coin");
  if (!coin_int) {
    return nullptr;
  }
  auto coin = static_cast<mojom::CoinType>(*coin_int);
  if (!mojom::IsKnownEnumValue(coin)) {
    return nullptr;
  }
  token_ptr->coin = coin;

  auto* chain_id = value.FindString("chain_id");
  if (!chain_id) {
    return nullptr;
  }
  token_ptr->chain_id = *chain_id;

  const std::string* contract_address = value.FindString("address");
  if (!contract_address) {
    return nullptr;
  }
  token_ptr->contract_address = *contract_address;

  const std::string* name = value.FindString("name");
  if (!name) {
    return nullptr;
  }
  token_ptr->name = *name;

  const std::string* symbol = value.FindString("symbol");
  if (!symbol) {
    return nullptr;
  }
  token_ptr->symbol = *symbol;

  const std::string* logo = value.FindString("logo");
  if (logo) {
    token_ptr->logo = *logo;
  }

  std::optional<bool> is_erc20 = value.FindBool("is_erc20");
  if (!is_erc20) {
    return nullptr;
  }
  token_ptr->is_erc20 = is_erc20.value();

  std::optional<bool> is_erc721 = value.FindBool("is_erc721");
  if (!is_erc721) {
    return nullptr;
  }
  token_ptr->is_erc721 = is_erc721.value();

  std::optional<bool> is_erc1155 = value.FindBool("is_erc1155");
  if (!is_erc1155) {
    // Might be missing in case of migration (03/2023).
    is_erc1155 = false;
  } else {
    token_ptr->is_erc1155 = is_erc1155.value();
  }

  std::optional<bool> is_spam = value.FindBool("is_spam");
  if (!is_spam) {
    // Might be missing in case of migration (06/2023).
    token_ptr->is_spam = false;
  } else {
    token_ptr->is_spam = is_spam.value();
  }

  // There might be existing pref values that does not have is_nft yet, in this
  // case, fallback to is_erc721 value.
  std::optional<bool> is_nft = value.FindBool("is_nft");
  if (is_nft) {
    token_ptr->is_nft = is_nft.value();
  } else {
    token_ptr->is_nft = token_ptr->is_erc721;
  }

  std::optional<int> decimals = value.FindInt("decimals");
  if (!decimals) {
    return nullptr;
  }
  token_ptr->decimals = decimals.value();

  std::optional<bool> visible = value.FindBool("visible");
  if (!visible) {
    return nullptr;
  }
  token_ptr->visible = visible.value();

  const std::string* token_id = value.FindString("token_id");
  if (token_id) {
    token_ptr->token_id = *token_id;
  }

  const std::string* coingecko_id = value.FindString("coingecko_id");
  if (coingecko_id) {
    token_ptr->coingecko_id = *coingecko_id;
  }

  if (IsSPLToken(token_ptr)) {
    auto spl_token_program_int = value.FindInt("spl_token_program");
    if (!spl_token_program_int) {
      token_ptr->spl_token_program = mojom::SPLTokenProgram::kUnknown;
    } else {
      auto spl_token_program =
          static_cast<mojom::SPLTokenProgram>(*spl_token_program_int);
      token_ptr->spl_token_program = mojom::IsKnownEnumValue(spl_token_program)
                                         ? spl_token_program
                                         : mojom::SPLTokenProgram::kUnknown;
    }
  } else {
    token_ptr->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
  }
  token_ptr->is_compressed = value.FindBool("is_compressed").value_or(false);

  return token_ptr;
}

base::Value::Dict BlockchainTokenToValue(
    const mojom::BlockchainTokenPtr& token) {
  CHECK(token);
  base::Value::Dict value;
  value.Set("address", token->contract_address);
  value.Set("name", token->name);
  value.Set("symbol", token->symbol);
  value.Set("logo", token->logo);
  value.Set("is_erc20", token->is_erc20);
  value.Set("is_erc721", token->is_erc721);
  value.Set("is_erc1155", token->is_erc1155);
  value.Set("is_nft", token->is_nft);
  value.Set("is_spam", token->is_spam);
  value.Set("decimals", token->decimals);
  value.Set("visible", token->visible);
  value.Set("token_id", token->token_id);
  value.Set("coingecko_id", token->coingecko_id);
  value.Set("coin", static_cast<int>(token->coin));
  value.Set("chain_id", token->chain_id);
  value.Set("spl_token_program", static_cast<int>(token->spl_token_program));
  value.Set("is_compressed", token->is_compressed);
  return value;
}

// Creates a response object as described in:
// https://eips.ethereum.org/EIPS/eip-2255
base::Value::List PermissionRequestResponseToValue(
    const url::Origin& origin,
    const std::vector<std::string>& accounts) {
  base::Value::List container_list;
  base::Value::Dict dict;
  dict.Set("id", base::Uuid::GenerateRandomV4().AsLowercaseString());

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
    filter_response_list.Append(base::ToLowerASCII(account));
  }
  caveats_obj2.Set("value", std::move(filter_response_list));
  caveats_list.Append(std::move(caveats_obj2));
  dict.Set("caveats", std::move(caveats_list));

  dict.Set("date", base::Time::Now().InMillisecondsFSinceUnixEpoch());
  dict.Set("invoker", origin.Serialize());
  dict.Set("parentCapability", "eth_accounts");
  container_list.Append(std::move(dict));
  return container_list;
}

int GetFirstValidChainURLIndex(const std::vector<GURL>& chain_urls) {
  if (chain_urls.empty()) {
    return 0;
  }
  size_t index = 0;
  for (const GURL& url : chain_urls) {
    if (net::IsHTTPSOrLocalhostURL(url) &&
        !base::Contains(url.spec(), "$%7BINFURA_API_KEY%7D") &&
        !base::Contains(url.spec(), "$%7BALCHEMY_API_KEY%7D") &&
        !base::Contains(url.spec(), "$%7BAPI_KEY%7D") &&
        !base::Contains(url.spec(), "$%7BPULSECHAIN_API_KEY%7D")) {
      return index;
    }
    index++;
  }
  return 0;
}

bool ReadUint32StringTo(const base::Value::Dict& dict,
                        std::string_view key,
                        uint32_t& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  return base::StringToUint(*str, &to);
}

bool ReadStringTo(const base::Value::Dict& dict,
                  std::string_view key,
                  std::string& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  to = *str;
  return true;
}

bool ReadUint64StringTo(const base::Value::Dict& dict,
                        std::string_view key,
                        uint64_t& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  return base::StringToUint64(*str, &to);
}

bool ReadHexByteArrayTo(const base::Value::Dict& dict,
                        std::string_view key,
                        std::vector<uint8_t>& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  if (str->empty()) {
    to.clear();
    return true;
  }
  return base::HexStringToBytes(*str, &to);
}

}  // namespace brave_wallet
