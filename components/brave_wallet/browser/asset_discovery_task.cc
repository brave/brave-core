/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_task.h"

#include <map>
#include <optional>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/environment.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/string_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "components/prefs/pref_service.h"
#include "net/base/url_util.h"

namespace brave_wallet {

AssetDiscoveryTask::AssetDiscoveryTask(APIRequestHelper& api_request_helper,
                                       SimpleHashClient& simple_hash_client,
                                       BraveWalletService& wallet_service,
                                       JsonRpcService& json_rpc_service,
                                       PrefService* prefs)
    : api_request_helper_(api_request_helper),
      simple_hash_client_(simple_hash_client),
      wallet_service_(wallet_service),
      json_rpc_service_(json_rpc_service),
      prefs_(prefs),
      weak_ptr_factory_(this) {}

AssetDiscoveryTask::~AssetDiscoveryTask() = default;

void AssetDiscoveryTask::ScheduleTask(
    const std::map<mojom::CoinType, std::vector<std::string>>&
        fungible_chain_ids,
    const std::map<mojom::CoinType, std::vector<std::string>>&
        non_fungible_chain_ids,
    const std::map<mojom::CoinType, std::vector<std::string>>&
        account_addresses,
    base::OnceClosure callback) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&AssetDiscoveryTask::DiscoverAssets,
                                weak_ptr_factory_.GetWeakPtr(),
                                fungible_chain_ids, non_fungible_chain_ids,
                                account_addresses, std::move(callback)));
}

void AssetDiscoveryTask::DiscoverAssets(
    const std::map<mojom::CoinType, std::vector<std::string>>&
        fungible_chain_ids,
    const std::map<mojom::CoinType, std::vector<std::string>>&
        non_fungible_chain_ids,
    const std::map<mojom::CoinType, std::vector<std::string>>&
        account_addresses,
    base::OnceClosure callback) {
  // Notify frontend asset discovery has started
  wallet_service_->OnDiscoverAssetsStarted();

  // Create list of accounts and chain IDs to be used as arguments
  auto sol_it = account_addresses.find(mojom::CoinType::SOL);
  const auto& sol_account_addresses = sol_it != account_addresses.end()
                                          ? sol_it->second
                                          : std::vector<std::string>();
  auto eth_it = account_addresses.find(mojom::CoinType::ETH);
  const auto& eth_account_addresses = eth_it != account_addresses.end()
                                          ? eth_it->second
                                          : std::vector<std::string>();
  eth_it = fungible_chain_ids.find(mojom::CoinType::ETH);
  const auto& eth_chain_ids = eth_it != fungible_chain_ids.end()
                                  ? eth_it->second
                                  : std::vector<std::string>();

  const auto& ankr_blockchains = GetAnkrBlockchains();
  std::vector<std::string> ankr_evm_chain_ids;
  std::vector<std::string> non_ankr_evm_chain_ids;
  for (const auto& chain_id : eth_chain_ids) {
    if (ankr_blockchains.contains(chain_id)) {
      ankr_evm_chain_ids.push_back(chain_id);
    } else {
      non_ankr_evm_chain_ids.push_back(chain_id);
    }
  }

  bool use_ankr_discovery =
      IsAnkrBalancesEnabled() && !ankr_evm_chain_ids.empty();

  // Concurrently discover ETH ERC20s on our registry, Solana tokens on our
  // Registry and NFTs on both platforms, then merge the results
  const auto barrier_callback =
      base::BarrierCallback<std::vector<mojom::BlockchainTokenPtr>>(
          use_ankr_discovery ? 4 : 3,
          base::BindOnce(&AssetDiscoveryTask::MergeDiscoveredAssets,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  // Currently SPL tokens are only discovered on Solana Mainnet.
  DiscoverSPLTokensFromRegistry(sol_account_addresses, barrier_callback);

  if (use_ankr_discovery) {
    DiscoverAnkrTokens(ankr_evm_chain_ids, eth_account_addresses,
                       barrier_callback);
    DiscoverERC20sFromRegistry(non_ankr_evm_chain_ids, eth_account_addresses,
                               barrier_callback);
  } else {
    DiscoverERC20sFromRegistry(eth_chain_ids, eth_account_addresses,
                               barrier_callback);
  }

  DiscoverNFTs(non_fungible_chain_ids, account_addresses, barrier_callback);
}

void AssetDiscoveryTask::MergeDiscoveredAssets(
    base::OnceClosure callback,
    const std::vector<std::vector<mojom::BlockchainTokenPtr>>&
        discovered_assets_lists) {
  std::vector<mojom::BlockchainTokenPtr> flattened_assets;
  for (const auto& list : discovered_assets_lists) {
    for (const auto& asset : list) {
      flattened_assets.push_back(asset.Clone());
    }
  }

  wallet_service_->OnDiscoverAssetsCompleted(std::move(flattened_assets));
  std::move(callback).Run();
}

void AssetDiscoveryTask::DiscoverAnkrTokens(
    const std::vector<std::string>& chain_ids,
    const std::vector<std::string>& account_addresses,
    DiscoverAssetsCompletedCallback callback) {
  if (account_addresses.empty() || chain_ids.empty()) {
    std::move(callback).Run({});
    return;
  }

  // Use a barrier callback to wait for all AnkrGetAccountBalances calls to
  // complete (one for each account address).
  const auto barrier_callback =
      base::BarrierCallback<std::vector<mojom::AnkrAssetBalancePtr>>(
          account_addresses.size(),
          base::BindOnce(&AssetDiscoveryTask::MergeDiscoveredAnkrTokens,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));

  // For each account address, call AnkrGetAccountBalances
  for (const auto& account_address : account_addresses) {
    auto internal_callback =
        base::BindOnce(&AssetDiscoveryTask::OnAnkrGetAccountBalances,
                       weak_ptr_factory_.GetWeakPtr(), barrier_callback);
    json_rpc_service_->AnkrGetAccountBalances(account_address, chain_ids,
                                              std::move(internal_callback));
  }
}

void AssetDiscoveryTask::OnAnkrGetAccountBalances(
    base::OnceCallback<void(std::vector<mojom::AnkrAssetBalancePtr>)>
        barrier_callback,
    std::vector<mojom::AnkrAssetBalancePtr> balances,
    mojom::ProviderError error,
    const std::string& error_message) {
  // If the request failed, return an empty vector
  if (error != mojom::ProviderError::kSuccess || !error_message.empty()) {
    std::move(barrier_callback).Run({});
    return;
  }

  std::move(barrier_callback).Run(std::move(balances));
}

void AssetDiscoveryTask::MergeDiscoveredAnkrTokens(
    DiscoverAssetsCompletedCallback callback,
    const std::vector<std::vector<mojom::AnkrAssetBalancePtr>>&
        discovered_assets_results) {
  // Create a vector of BlockchainTokenPtrs to return
  std::vector<mojom::BlockchainTokenPtr> discovered_tokens;

  for (const auto& discovered_assets_result : discovered_assets_results) {
    for (const auto& balance : discovered_assets_result) {
      DCHECK(balance->asset->visible);
      if (!AddUserAsset(prefs_, balance->asset.Clone())) {
        continue;
      }
      discovered_tokens.push_back(balance->asset.Clone());
    }
  }

  std::move(callback).Run(std::move(discovered_tokens));
}

void AssetDiscoveryTask::DiscoverERC20sFromRegistry(
    const std::vector<std::string>& chain_ids,
    const std::vector<std::string>& account_addresses,
    DiscoverAssetsCompletedCallback callback) {
  if (account_addresses.empty()) {
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::BlockchainTokenPtr> user_assets = GetAllUserAssets(prefs_);
  TokenListMap token_list_map =
      BlockchainRegistry::GetInstance()->GetEthTokenListMap(chain_ids);

  // Create set of all user assets per chain to use to ensure we don't
  // include assets the user has already added in the call to the BalanceScanner
  base::flat_map<std::string, base::flat_set<std::string_view>>
      user_assets_per_chain;
  for (const auto& user_asset : user_assets) {
    user_assets_per_chain[user_asset->chain_id].insert(
        user_asset->contract_address);
  }

  // Create a map of chain_id to a map contract address to BlockchainToken
  // to easily lookup tokens by contract address when the results of the
  // BalanceScanner calls are merged
  base::flat_map<std::string,
                 base::flat_map<std::string, mojom::BlockchainTokenPtr>>
      chain_id_to_contract_address_to_token;

  // Create a map of chain_id to a vector of contract addresses (strings, rather
  // than BlockchainTokens) to pass to GetERC20TokenBalances
  base::flat_map<std::string, std::vector<std::string>>
      chain_id_to_contract_addresses;

  // Populate the chain_id_to_contract_addresses using the token_list_map of
  // BlockchainTokenPtrs
  for (auto& [chain_id, token_list] : token_list_map) {
    for (auto& token : token_list) {
      if (!user_assets_per_chain[chain_id].contains(token->contract_address)) {
        chain_id_to_contract_addresses[chain_id].push_back(
            token->contract_address);
        chain_id_to_contract_address_to_token[chain_id]
                                             [token->contract_address] =
                                                 std::move(token);
      }
    }
  }

  // Use a barrier callback to wait for all GetERC20TokenBalances calls to
  // complete (one for each account address).
  const auto barrier_callback =
      base::BarrierCallback<std::map<std::string, std::vector<std::string>>>(
          account_addresses.size() * chain_id_to_contract_addresses.size(),
          base::BindOnce(&AssetDiscoveryTask::MergeDiscoveredERC20s,
                         weak_ptr_factory_.GetWeakPtr(),
                         std::move(chain_id_to_contract_address_to_token),
                         std::move(callback)));

  // For each account address, call GetERC20TokenBalances for each chain ID
  for (const auto& account_address : account_addresses) {
    for (const auto& [chain_id, contract_addresses] :
         chain_id_to_contract_addresses) {
      auto internal_callback =
          base::BindOnce(&AssetDiscoveryTask::OnGetERC20TokenBalances,
                         weak_ptr_factory_.GetWeakPtr(), barrier_callback,
                         chain_id, contract_addresses);
      json_rpc_service_->GetERC20TokenBalances(contract_addresses,
                                               account_address, chain_id,
                                               std::move(internal_callback));
    }
  }
}

void AssetDiscoveryTask::OnGetERC20TokenBalances(
    base::OnceCallback<void(std::map<std::string, std::vector<std::string>>)>
        barrier_callback,
    const std::string& chain_id,
    const std::vector<std::string>&
        contract_addresses,  // Contract addresses queried for
    std::vector<mojom::ERC20BalanceResultPtr> balance_results,
    mojom::ProviderError error,
    const std::string& error_message) {
  // If the request failed, return an empty map
  if (error != mojom::ProviderError::kSuccess || balance_results.empty()) {
    std::move(barrier_callback).Run({});
    return;
  }

  // Create a map of chain_id to a vector of contract addresses that have a
  // balance greater than 0
  std::map<std::string, std::vector<std::string>>
      chain_id_to_contract_addresses_with_balance;

  // Populate the map using the balance_results
  for (size_t i = 0; i < balance_results.size(); i++) {
    if (balance_results[i]->balance.has_value()) {
      uint256_t balance_uint;
      bool success =
          HexValueToUint256(balance_results[i]->balance.value(), &balance_uint);
      if (success && balance_uint > 0) {
        chain_id_to_contract_addresses_with_balance[chain_id].push_back(
            contract_addresses[i]);
      }
    }
  }

  std::move(barrier_callback).Run(chain_id_to_contract_addresses_with_balance);
}

void AssetDiscoveryTask::MergeDiscoveredERC20s(
    base::flat_map<std::string,
                   base::flat_map<std::string, mojom::BlockchainTokenPtr>>
        chain_id_to_contract_address_to_token,
    DiscoverAssetsCompletedCallback callback,
    const std::vector<std::map<std::string, std::vector<std::string>>>&
        discovered_assets_results) {
  // Create a vector of BlockchainTokenPtrs to return
  std::vector<mojom::BlockchainTokenPtr> discovered_tokens;

  // Keep track of which contract addresses have been seen per chain
  base::flat_map<std::string, base::flat_set<std::string_view>>
      seen_contract_addresses;
  for (const auto& discovered_assets_result : discovered_assets_results) {
    for (const auto& [chain_id, contract_addresses] :
         discovered_assets_result) {
      for (const auto& contract_address : contract_addresses) {
        // Skip if seen
        if (seen_contract_addresses[chain_id].contains(contract_address)) {
          continue;
        }

        // Add to seen and discovered_tokens if not
        seen_contract_addresses[chain_id].insert(contract_address);
        auto token = std::move(
            chain_id_to_contract_address_to_token[chain_id][contract_address]);
        if (!token) {
          continue;
        }

        DCHECK(token->visible);
        if (AddUserAsset(prefs_, token.Clone())) {
          discovered_tokens.push_back(std::move(token));
        }
      }
    }
  }

  std::move(callback).Run(std::move(discovered_tokens));
}

void AssetDiscoveryTask::DiscoverSPLTokensFromRegistry(
    const std::vector<std::string>& account_addresses,
    DiscoverAssetsCompletedCallback callback) {
  // Convert each account address to SolanaAddress and check validity
  std::vector<SolanaAddress> solana_addresses;
  for (const auto& address : account_addresses) {
    std::optional<SolanaAddress> solana_address =
        SolanaAddress::FromBase58(address);
    if (!solana_address.has_value()) {
      continue;
    }

    if ((*solana_address).IsValid()) {
      solana_addresses.push_back(*solana_address);
    }
  }

  if (solana_addresses.empty()) {
    std::move(callback).Run({});
    return;
  }

  const auto barrier_callback =
      base::BarrierCallback<std::vector<SolanaAddress>>(
          solana_addresses.size(),
          base::BindOnce(&AssetDiscoveryTask::MergeDiscoveredSPLTokens,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  for (const auto& account_address : solana_addresses) {
    // Solana Mainnet is the only network supported currently
    json_rpc_service_->GetSolanaTokenAccountsByOwner(
        account_address, mojom::kSolanaMainnet,
        base::BindOnce(&AssetDiscoveryTask::OnGetSolanaTokenAccountsByOwner,
                       weak_ptr_factory_.GetWeakPtr(), barrier_callback));
  }
}

void AssetDiscoveryTask::OnGetSolanaTokenAccountsByOwner(
    base::OnceCallback<void(std::vector<SolanaAddress>)> barrier_callback,
    std::vector<SolanaAccountInfo> token_accounts,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess || token_accounts.empty()) {
    std::move(barrier_callback).Run(std::vector<SolanaAddress>());
    return;
  }

  // Add each token account to the all_discovered_contract_addresses list
  std::vector<SolanaAddress> discovered_mint_addresses;
  for (const auto& token_account : token_accounts) {
    // Decode Base64
    const std::optional<std::vector<uint8_t>> data =
        base::Base64Decode(token_account.data);
    if (data.has_value()) {
      // Decode the address
      const std::optional<SolanaAddress> mint_address =
          DecodeMintAddress(data.value());
      if (mint_address.has_value()) {
        // Add the contract address to the list
        discovered_mint_addresses.push_back(mint_address.value());
      }
    }
  }

  std::move(barrier_callback).Run(discovered_mint_addresses);
}

void AssetDiscoveryTask::MergeDiscoveredSPLTokens(
    DiscoverAssetsCompletedCallback callback,
    const std::vector<std::vector<SolanaAddress>>&
        all_discovered_contract_addresses) {
  // Create vector of all discovered contract addresses
  std::vector<std::string> discovered_mint_addresses;
  for (const auto& discovered_contract_address_list :
       all_discovered_contract_addresses) {
    for (const auto& discovered_contract_address :
         discovered_contract_address_list) {
      discovered_mint_addresses.push_back(
          discovered_contract_address.ToBase58());
    }
  }

  // Convert vector to flat_set
  base::flat_set<std::string> discovered_mint_addresses_set(
      std::move(discovered_mint_addresses));

  auto internal_callback =
      base::BindOnce(&AssetDiscoveryTask::OnGetSolanaTokenRegistry,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(discovered_mint_addresses_set));

  // Fetch SOL registry tokens (mainnet only).
  // TODO(nvonpentz) This needs to be changed when we support multiple chains
  // for Solana.
  BlockchainRegistry::GetInstance()->GetAllTokens(mojom::kSolanaMainnet,
                                                  mojom::CoinType::SOL,
                                                  std::move(internal_callback));
}

void AssetDiscoveryTask::OnGetSolanaTokenRegistry(
    DiscoverAssetsCompletedCallback callback,
    const base::flat_set<std::string>& discovered_mint_addresses,
    std::vector<mojom::BlockchainTokenPtr> sol_token_registry) {
  std::vector<mojom::BlockchainTokenPtr> discovered_tokens;
  for (const auto& token : sol_token_registry) {
    if (discovered_mint_addresses.contains(token->contract_address)) {
      DCHECK(token->visible);
      if (!AddUserAsset(prefs_, token.Clone())) {
        continue;
      }
      discovered_tokens.push_back(token.Clone());
    }
  }

  std::move(callback).Run(std::move(discovered_tokens));
}

void AssetDiscoveryTask::DiscoverNFTs(
    const std::map<mojom::CoinType, std::vector<std::string>>& chain_ids,
    const std::map<mojom::CoinType, std::vector<std::string>>&
        account_addresses,
    DiscoverAssetsCompletedCallback callback) {
  // Users must opt-in for NFT discovery
  if (!prefs_->GetBoolean(kBraveWalletNftDiscoveryEnabled)) {
    std::move(callback).Run({});
    return;
  }

  auto it_eth = account_addresses.find(mojom::CoinType::ETH);
  const auto& eth_account_addresses = it_eth != account_addresses.end()
                                          ? it_eth->second
                                          : std::vector<std::string>();
  auto it_sol = account_addresses.find(mojom::CoinType::SOL);
  const auto& sol_account_addresses = it_sol != account_addresses.end()
                                          ? it_sol->second
                                          : std::vector<std::string>();
  const auto barrier_callback =
      base::BarrierCallback<std::vector<mojom::BlockchainTokenPtr>>(
          eth_account_addresses.size() + sol_account_addresses.size(),
          base::BindOnce(&AssetDiscoveryTask::MergeDiscoveredNFTs,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  for (const auto& account_address : eth_account_addresses) {
    simple_hash_client_->FetchAllNFTsFromSimpleHash(
        account_address, chain_ids.at(mojom::CoinType::ETH),
        mojom::CoinType::ETH, barrier_callback);
  }

  for (const auto& account_address : sol_account_addresses) {
    simple_hash_client_->FetchAllNFTsFromSimpleHash(
        account_address, chain_ids.at(mojom::CoinType::SOL),
        mojom::CoinType::SOL, barrier_callback);
  }
}

void AssetDiscoveryTask::MergeDiscoveredNFTs(
    DiscoverAssetsCompletedCallback callback,
    const std::vector<std::vector<mojom::BlockchainTokenPtr>>& nfts) {
  // De-dupe the NFTs
  base::flat_set<mojom::BlockchainTokenPtr> seen_nft;
  std::vector<mojom::BlockchainTokenPtr> discovered_nfts;
  for (const auto& nft_list : nfts) {
    for (const auto& nft : nft_list) {
      if (seen_nft.contains(nft)) {
        continue;
      }
      seen_nft.insert(nft.Clone());

      // Add the NFT to the user's assets
      DCHECK(nft->visible);
      if (AddUserAsset(prefs_, nft.Clone())) {
        discovered_nfts.push_back(nft.Clone());
      }
    }
  }

  std::move(callback).Run(std::move(discovered_nfts));
}

// static
// Parses the Account object for the `mint` field which is a 32 byte public key.
// See
// https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L86-L105
std::optional<SolanaAddress> AssetDiscoveryTask::DecodeMintAddress(
    const std::vector<uint8_t>& data) {
  if (data.size() < 32) {
    return std::nullopt;
  }

  std::vector<uint8_t> pub_key_bytes(data.begin(), data.begin() + 32);
  return SolanaAddress::FromBytes(pub_key_bytes);
}

}  // namespace brave_wallet
