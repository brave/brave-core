/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_registry.h"

#include <algorithm>
#include <utility>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/json/rs/src/lib.rs.h"
#include "net/base/url_util.h"
#include "services/data_decoder/public/cpp/json_sanitizer.h"

namespace brave_wallet {

namespace {

void HandleRampTokenLists(const absl::optional<std::string>& result) {
  if (!result.has_value()) {
    return;
  }
  auto parsedRampTokensListMaps = ParseRampTokenListMaps(*result);
  if (!parsedRampTokensListMaps) {
    VLOG(1) << "Can't parse on/off ramp token lists.";
    return;
  }

  if (parsedRampTokensListMaps->first.empty()) {
    VLOG(1) << "On ramp supported token lists is empty.";
  } else {
    BlockchainRegistry::GetInstance()->UpdateOnRampTokenLists(
        std::move(parsedRampTokensListMaps->first));
  }

  if (parsedRampTokensListMaps->second.empty()) {
    VLOG(1) << "Off ramp supported sell token lists is empty.";
  } else {
    BlockchainRegistry::GetInstance()->UpdateOffRampTokenLists(
        std::move(parsedRampTokensListMaps->second));
  }
}

void HandleOnRampCurrenciesLists(const absl::optional<std::string>& result) {
  if (!result.has_value()) {
    return;
  }

  absl::optional<std::vector<mojom::OnRampCurrency>> lists =
      ParseOnRampCurrencyLists(*result);
  if (!lists) {
    VLOG(1) << "Can't parse on ramp supported sell token lists.";
    return;
  }

  BlockchainRegistry::GetInstance()->UpdateOnRampCurrenciesLists(
      std::move(*lists));
}

base::FilePath ResolveAbsolutePath(const base::FilePath& input_path) {
  // On some platforms (e.g. Mac) we use symlinks for paths. Convert paths to
  // absolute paths to avoid unexpected failure. base::MakeAbsoluteFilePath()
  // requires IO so it can only be done in this function and be posted to the
  // sequenced runner.
  const base::FilePath output_path = base::MakeAbsoluteFilePath(input_path);

  if (output_path.empty()) {
    LOG(ERROR) << "Failed to get absolute install path.";
  }

  return output_path;
}

absl::optional<std::string> ParseJsonFile(base::FilePath path,
                                          const std::string& filename) {
  // We do not sanitize the result here via JsonSanitizer::Sanitize to optimize
  // the performance because we are processing data from our own CRX downloaded
  // via component updater, hence it is considered as trusted input.
  // See https://github.com/brave/brave-browser/issues/30940 for details.
  std::string json_content;
  const base::FilePath json_path = path.AppendASCII(filename);
  if (!base::ReadFileToString(json_path, &json_content)) {
    LOG(ERROR) << "Can't read file: " << filename;
    return absl::nullopt;
  }

  return json_content;
}

void ParseCoingeckoIdsMapAndUpdateRegistry(const base::FilePath& dir) {
  auto result = ParseJsonFile(dir, "coingecko-ids.json");
  if (!result) {
    return;
  }

  absl::optional<CoingeckoIdsMap> coingecko_ids_map =
      ParseCoingeckoIdsMap(*result);
  if (!coingecko_ids_map) {
    VLOG(1) << "Can't parse coingecko-ids.json";
    return;
  }

  BlockchainRegistry::GetInstance()->UpdateCoingeckoIdsMap(
      std::move(*coingecko_ids_map));
}

void HandleParseTokenList(const base::FilePath& dir,
                          const std::string& filename,
                          mojom::CoinType coin_type) {
  auto result = ParseJsonFile(dir, filename);
  if (!result) {
    return;
  }

  TokenListMap lists;
  if (!ParseTokenList(*result, &lists, coin_type)) {
    VLOG(1) << "Can't parse token list.";
    return;
  }

  for (auto& list_pair : lists) {
    BlockchainRegistry::GetInstance()->UpdateTokenList(
        list_pair.first, std::move(list_pair.second));
  }
}

void ParseTokenListAndUpdateRegistry(const base::FilePath& dir) {
  HandleParseTokenList(dir, "contract-map.json", mojom::CoinType::ETH);
  HandleParseTokenList(dir, "evm-contract-map.json", mojom::CoinType::ETH);
  HandleParseTokenList(dir, "solana-contract-map.json", mojom::CoinType::SOL);
}

void ParseChainListAndUpdateRegistry(const base::FilePath& dir) {
  auto result = ParseJsonFile(dir, "chainlist.json");
  if (!result) {
    return;
  }

  ChainList chains;
  if (!ParseChainList(*result, &chains)) {
    VLOG(1) << "Can't parse chain list.";
    return;
  }

  BlockchainRegistry::GetInstance()->UpdateChainList(std::move(chains));
}

void ParseDappListsAndUpdateRegistry(const base::FilePath& dir) {
  auto result = ParseJsonFile(dir, "dapp-lists.json");
  if (!result) {
    return;
  }

  auto converted_json =
      std::string(json::convert_all_numbers_to_string(*result, ""));
  if (converted_json.empty()) {
    return;
  }

  absl::optional<DappListMap> lists = ParseDappLists(converted_json);
  if (!lists) {
    VLOG(1) << "Can't parse dapp lists.";
    return;
  }

  BlockchainRegistry::GetInstance()->UpdateDappList(std::move(*lists));
}

void ParseOnRampListsAndUpdateRegistry(const base::FilePath& dir) {
  auto result = ParseJsonFile(dir, "ramp-tokens.json");
  HandleRampTokenLists(result);

  result = ParseJsonFile(dir, "on-ramp-currency-lists.json");
  HandleOnRampCurrenciesLists(result);
}

void ParseOfacAddressesListsAndUpdateRegistry(const base::FilePath& dir) {
  auto result =
      ParseJsonFile(dir, "ofac-sanctioned-digital-currency-addresses.json");
  if (!result) {
    return;
  }

  absl::optional<std::vector<std::string>> list =
      ParseOfacAddressesList(*result);
  if (!list) {
    VLOG(1) << "Can't parse ofac addresses list.";
    return;
  }

  BlockchainRegistry::GetInstance()->UpdateOfacAddressesList(std::move(*list));
}

}  // namespace

BlockchainRegistry::BlockchainRegistry() {
  sequenced_task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
}

BlockchainRegistry::~BlockchainRegistry() = default;

BlockchainRegistry* BlockchainRegistry::GetInstance() {
  static base::NoDestructor<BlockchainRegistry> instance;
  return instance.get();
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

void BlockchainRegistry::UpdateCoingeckoIdsMap(
    CoingeckoIdsMap coingecko_ids_map) {
  coingecko_ids_map_ = std::move(coingecko_ids_map);
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

void BlockchainRegistry::UpdateOnRampTokenLists(
    OnRampTokensListMap on_ramp_lists) {
  on_ramp_token_lists_ = std::move(on_ramp_lists);
}

void BlockchainRegistry::UpdateOffRampTokenLists(
    OffRampTokensListMap on_ramp_lists) {
  off_ramp_token_lists_ = std::move(on_ramp_lists);
}

void BlockchainRegistry::UpdateOnRampCurrenciesLists(
    std::vector<mojom::OnRampCurrency> on_ramp_currencies_list) {
  on_ramp_currencies_list_ = std::move(on_ramp_currencies_list);
}

void BlockchainRegistry::UpdateOfacAddressesList(
    std::vector<std::string> ofac_addresses_list) {
  ofac_addresses_ = base::flat_set<std::string>(ofac_addresses_list.begin(),
                                                ofac_addresses_list.end());
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
    if (!on_ramp_token_lists_.contains(provider)) {
      continue;
    }
    const std::vector<mojom::BlockchainTokenPtr>& buy_tokens =
        on_ramp_token_lists_[provider];
    for (const auto& token : buy_tokens) {
      if (token->chain_id == chain_id) {
        blockchain_buy_tokens.push_back(token.Clone());
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

  auto sell_tokens_itr = off_ramp_token_lists_.find(provider);

  // Check if the provider is found in the map
  if (sell_tokens_itr == off_ramp_token_lists_.end()) {
    std::move(callback).Run(std::move(blockchain_sell_tokens));
    return;
  }

  // Now we have a vector of BlockchainTokens for the provider
  auto& sell_tokens = sell_tokens_itr->second;

  // Filter out tokens that do not belong to the specified chain_id
  for (const auto& token : sell_tokens) {
    if (token->chain_id != chain_id) {
      continue;
    }

    blockchain_sell_tokens.push_back(mojom::BlockchainToken::New(*token));
  }

  std::move(callback).Run(std::move(blockchain_sell_tokens));
}

void BlockchainRegistry::GetOnRampCurrencies(
    GetOnRampCurrenciesCallback callback) {
  std::vector<mojom::OnRampCurrencyPtr> currencies;

  for (const auto& currency : on_ramp_currencies_list_) {
    currencies.push_back(currency.Clone());
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

void BlockchainRegistry::GetTopDapps(const std::string& chain_id,
                                     mojom::CoinType coin,
                                     GetTopDappsCallback callback) {
  const auto key = GetTokenListKey(coin, chain_id);
  if (!dapp_lists_.contains(key)) {
    std::move(callback).Run(std::vector<mojom::DappPtr>());
    return;
  }
  const auto& dapps = dapp_lists_[key];

  std::vector<mojom::DappPtr> dapps_copy(dapps.size());
  std::transform(dapps.begin(), dapps.end(), dapps_copy.begin(),
                 [](const mojom::DappPtr& current_token) -> mojom::DappPtr {
                   return current_token.Clone();
                 });
  std::move(callback).Run(std::move(dapps_copy));
}

absl::optional<std::string> BlockchainRegistry::GetCoingeckoId(
    const std::string& chain_id,
    const std::string& contract_address) {
  const auto& chain_id_lower = base::ToLowerASCII(chain_id);
  const auto& contract_address_lower = base::ToLowerASCII(contract_address);

  if (!coingecko_ids_map_.contains({chain_id_lower, contract_address_lower})) {
    return absl::nullopt;
  }

  return coingecko_ids_map_[{chain_id_lower, contract_address_lower}];
}

void BlockchainRegistry::GetCoingeckoId(const std::string& chain_id,
                                        const std::string& contract_address,
                                        GetCoingeckoIdCallback callback) {
  std::move(callback).Run(GetCoingeckoId(chain_id, contract_address));
}

bool BlockchainRegistry::IsOfacAddress(const std::string& address) {
  return base::Contains(ofac_addresses_, base::ToLowerASCII(address));
}

void BlockchainRegistry::ParseLists(const base::FilePath& install_dir,
                                    base::OnceClosure callback) {
  if (install_dir.empty()) {
    std::move(callback).Run();
    return;
  }

  // ResolveAbsolutePath needs to be posted due to I/O operation.
  // base::Unretained(this) is safe here due to NoDestructor.
  sequenced_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&ResolveAbsolutePath, install_dir),
      base::BindOnce(&BlockchainRegistry::DoParseLists, base::Unretained(this),
                     std::move(callback)));
}

void BlockchainRegistry::DoParseLists(
    base::OnceClosure callback,
    const base::FilePath& absolute_install_dir) {
  if (absolute_install_dir.empty()) {
    std::move(callback).Run();
    return;
  }

  // These parsing function needs to be posted due to reading JSON files.
  sequenced_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(
          [](const base::FilePath& absolute_install_dir) {
            ParseCoingeckoIdsMapAndUpdateRegistry(absolute_install_dir);
            ParseTokenListAndUpdateRegistry(absolute_install_dir);
            ParseChainListAndUpdateRegistry(absolute_install_dir);
            ParseDappListsAndUpdateRegistry(absolute_install_dir);
            ParseOnRampListsAndUpdateRegistry(absolute_install_dir);
            ParseOfacAddressesListsAndUpdateRegistry(absolute_install_dir);
          },
          absolute_install_dir),
      std::move(callback));
}

}  // namespace brave_wallet
