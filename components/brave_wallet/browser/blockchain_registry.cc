/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_registry.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

namespace {

struct ParseListsResult {
  CoingeckoIdsMap coingecko_ids_map;
  TokenListMap token_list_map;
  ChainList chain_list;
  std::vector<std::string> restricted_addresses;
  PolkadotRpcUrlsMap polkadot_rpc_urls;
};

base::FilePath ResolveAbsolutePath(const base::FilePath& input_path) {
  // On some platforms (e.g. Mac) we use symlinks for paths. Convert paths to
  // absolute paths to avoid unexpected failure. base::MakeAbsoluteFilePath()
  // requires IO so it needs to be posted.
  return base::MakeAbsoluteFilePath(input_path);
}

std::optional<std::string> ParseJsonFile(base::FilePath path,
                                         const std::string& filename) {
  // We do not sanitize the result here via JsonSanitizer::Sanitize to optimize
  // the performance because we are processing data from our own CRX downloaded
  // via component updater, hence it is considered as trusted input.
  // See https://github.com/brave/brave-browser/issues/30940 for details.
  std::string json_content;
  const base::FilePath json_path = path.AppendASCII(filename);
  if (!base::ReadFileToString(json_path, &json_content)) {
    return std::nullopt;
  }

  return json_content;
}

void DoParseCoingeckoIdsMap(const base::FilePath& dir, ParseListsResult& out) {
  auto result = ParseJsonFile(dir, "coingecko-ids.json");
  if (!result) {
    return;
  }

  std::optional<CoingeckoIdsMap> coingecko_ids_map =
      ParseCoingeckoIdsMap(*result);
  if (!coingecko_ids_map) {
    return;
  }

  out.coingecko_ids_map = std::move(*coingecko_ids_map);
}

void DoParseTokenList(const base::FilePath& dir, ParseListsResult& out) {
  auto result = ParseJsonFile(dir, "coingecko.json");
  if (!result) {
    return;
  }

  TokenListMap lists;
  if (!ParseTokenList(*result, &lists)) {
    return;
  }

  for (auto& list_pair : lists) {
    out.token_list_map[list_pair.first] = std::move(list_pair.second);
  }
}

void DoParseChainList(const base::FilePath& dir, ParseListsResult& out) {
  auto result = ParseJsonFile(dir, "chainlist.json");
  if (!result) {
    return;
  }

  ChainList chains;
  if (!ParseChainList(*result, &chains)) {
    return;
  }

  out.chain_list = std::move(chains);
}

void DoParseRestrictedAddressesLists(const base::FilePath& dir,
                                     ParseListsResult& out) {
  auto result =
      ParseJsonFile(dir, BlockchainRegistry::kRestrictedAddressFileName);
  if (!result) {
    return;
  }

  std::optional<std::vector<std::string>> list =
      ParseRestrictedAddressesList(*result);
  if (!list) {
    return;
  }

  out.restricted_addresses = std::move(*list);
}

void DoParsePolkadotRpcUrls(const base::FilePath& dir, ParseListsResult& out) {
  auto result =
      ParseJsonFile(dir, BlockchainRegistry::kPolkadotRpcUrlsFileName);
  if (!result) {
    return;
  }

  std::optional<PolkadotRpcUrlsMap> rpc_urls = ParsePolkadotRpcUrls(*result);
  if (!rpc_urls) {
    return;
  }

  out.polkadot_rpc_urls = std::move(*rpc_urls);
}

void UpdateRegistry(base::OnceClosure callback, ParseListsResult result) {
  auto* registry = BlockchainRegistry::GetInstance();
  registry->UpdateCoingeckoIdsMap(std::move(result.coingecko_ids_map));
  registry->UpdateTokenList(std::move(result.token_list_map));
  registry->UpdateChainList(std::move(result.chain_list));
  registry->UpdateRestrictedAddressesList(
      std::move(result.restricted_addresses));
  registry->UpdatePolkadotRpcUrls(std::move(result.polkadot_rpc_urls));
  std::move(callback).Run();
}

ParseListsResult DoParseLists(const base::FilePath& install_dir) {
  const base::FilePath absolute_install_dir = ResolveAbsolutePath(install_dir);
  if (absolute_install_dir.empty()) {
    return {};
  }

  ParseListsResult result;
  DoParseCoingeckoIdsMap(absolute_install_dir, result);
  DoParseTokenList(absolute_install_dir, result);
  DoParseChainList(absolute_install_dir, result);
  DoParseRestrictedAddressesLists(absolute_install_dir, result);
  DoParsePolkadotRpcUrls(absolute_install_dir, result);
  return result;
}

}  // namespace

BlockchainRegistry::BlockchainRegistry() = default;

BlockchainRegistry::~BlockchainRegistry() = default;

BlockchainRegistry* BlockchainRegistry::GetInstance() {
  static base::NoDestructor<BlockchainRegistry> instance;
  DCHECK_CALLED_ON_VALID_SEQUENCE(instance->sequence_checker_);
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
    const std::string& key,
    std::vector<mojom::BlockchainTokenPtr> list) {
  token_list_map_[key] = std::move(list);
}

void BlockchainRegistry::UpdateChainList(ChainList chains) {
  chain_list_ = std::move(chains);
}

void BlockchainRegistry::UpdateRestrictedAddressesList(
    std::vector<std::string> restricted_addresses_list) {
  restricted_addresses_ = base::flat_set<std::string>(
      restricted_addresses_list.begin(), restricted_addresses_list.end());
}

void BlockchainRegistry::UpdatePolkadotRpcUrls(
    PolkadotRpcUrlsMap polkadot_rpc_urls) {
  polkadot_rpc_urls_ = std::move(polkadot_rpc_urls);
}

const PolkadotRpcUrlsMap& BlockchainRegistry::GetPolkadotRpcUrls() const {
  return polkadot_rpc_urls_;
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
  auto token_it = std::ranges::find_if(
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
  auto token_it = std::ranges::find_if(
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

std::vector<mojom::NetworkInfoPtr>
BlockchainRegistry::GetPrepopulatedNetworks() {
  std::vector<mojom::NetworkInfoPtr> result;
  for (auto& chain : chain_list_) {
    if (auto known_chain = NetworkManager::GetKnownChain(
            chain->chain_id, mojom::CoinType::ETH)) {
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

std::optional<std::string> BlockchainRegistry::GetCoingeckoId(
    const std::string& chain_id,
    const std::string& contract_address) {
  const auto& chain_id_lower = base::ToLowerASCII(chain_id);
  const auto& contract_address_lower = base::ToLowerASCII(contract_address);

  if (!coingecko_ids_map_.contains({chain_id_lower, contract_address_lower})) {
    return std::nullopt;
  }

  return coingecko_ids_map_[{chain_id_lower, contract_address_lower}];
}

void BlockchainRegistry::GetCoingeckoId(const std::string& chain_id,
                                        const std::string& contract_address,
                                        GetCoingeckoIdCallback callback) {
  std::move(callback).Run(GetCoingeckoId(chain_id, contract_address));
}

bool BlockchainRegistry::IsRestrictedAddress(const std::string& address) {
  return restricted_addresses_.contains(base::ToLowerASCII(address));
}

void BlockchainRegistry::ParseLists(const base::FilePath& install_dir,
                                    base::OnceClosure callback) {
  if (install_dir.empty()) {
    std::move(callback).Run();
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&DoParseLists, install_dir),
      base::BindOnce(&UpdateRegistry, std::move(callback)));
}

BlockchainRegistry::ScopedRestrictedAddressesForTesting::
    ScopedRestrictedAddressesForTesting(
        std::vector<std::string> restricted_addresses) {
  auto* registry = GetInstance();
  previous_addresses_ = registry->restricted_addresses_;
  registry->UpdateRestrictedAddressesList(std::move(restricted_addresses));
}

BlockchainRegistry::ScopedRestrictedAddressesForTesting::
    ~ScopedRestrictedAddressesForTesting() {
  auto* registry = GetInstance();
  registry->restricted_addresses_ = std::move(previous_addresses_);
}

bool BlockchainRegistry::IsEmptyForTesting() {
  return coingecko_ids_map_.empty() && token_list_map_.empty() &&
         chain_list_.empty() && restricted_addresses_.empty() &&
         polkadot_rpc_urls_.empty();
}

void BlockchainRegistry::ResetForTesting() {
  coingecko_ids_map_.clear();
  token_list_map_.clear();
  restricted_addresses_.clear();
  polkadot_rpc_urls_.clear();
  receivers_.Clear();
}

}  // namespace brave_wallet
