/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"

#include <map>
#include <utility>

#include "base/base64.h"
#include "base/strings/strcat.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_topics_builder.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

AssetDiscoveryManager::AssetDiscoveryManager(BraveWalletService* wallet_service,
                                             JsonRpcService* json_rpc_service,
                                             KeyringService* keyring_service,
                                             PrefService* prefs)
    : wallet_service_(wallet_service),
      json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      prefs_(prefs),
      weak_ptr_factory_(this) {
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
}

AssetDiscoveryManager::~AssetDiscoveryManager() = default;

const std::vector<std::string>&
AssetDiscoveryManager::GetAssetDiscoverySupportedEthChains() {
  if (supported_chains_for_testing_.size() > 0) {
    return supported_chains_for_testing_;
  }

  // Use the hardcoded list of BalanceScanner contract addresses to determine
  // which chains are supported.
  static const base::NoDestructor<std::vector<std::string>>
      asset_discovery_supported_chains([] {
        std::vector<std::string> supported_chains;
        for (const auto& entry : GetEthBalanceScannerContractAddresses()) {
          supported_chains.push_back(entry.first);
        }
        return supported_chains;
      }());
  return *asset_discovery_supported_chains;
}

void AssetDiscoveryManager::DiscoverSolAssets(
    const std::vector<std::string>& account_addresses,
    bool triggered_by_accounts_added) {
  // Convert each account address to SolanaAddress and check validity
  std::vector<SolanaAddress> solana_addresses;
  for (const auto& address : account_addresses) {
    absl::optional<SolanaAddress> solana_address =
        SolanaAddress::FromBase58(address);
    if (!solana_address.has_value()) {
      continue;
    }

    if ((*solana_address).IsValid()) {
      solana_addresses.push_back(*solana_address);
    }
  }

  if (solana_addresses.empty()) {
    CompleteDiscoverAssets(std::vector<mojom::BlockchainTokenPtr>(),
                           triggered_by_accounts_added);
    return;
  }

  // TODO(nvonpentz): When custom networks are supported, we need to check
  // that the active network is our own that supports this RPC call.

  const auto barrier_callback =
      base::BarrierCallback<std::vector<SolanaAddress>>(
          solana_addresses.size(),
          base::BindOnce(&AssetDiscoveryManager::MergeDiscoveredSolanaAssets,
                         weak_ptr_factory_.GetWeakPtr(),
                         triggered_by_accounts_added));
  for (const auto& account_address : solana_addresses) {
    json_rpc_service_->GetSolanaTokenAccountsByOwner(
        account_address,
        base::BindOnce(&AssetDiscoveryManager::OnGetSolanaTokenAccountsByOwner,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(barrier_callback)));
  }
}

void AssetDiscoveryManager::OnGetSolanaTokenAccountsByOwner(
    base::OnceCallback<void(std::vector<SolanaAddress>)> barrier_callback,
    const std::vector<SolanaAccountInfo>& token_accounts,
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
    const absl::optional<std::vector<uint8_t>> data =
        base::Base64Decode(token_account.data);
    if (data.has_value()) {
      // Decode the address
      const absl::optional<SolanaAddress> mint_address =
          DecodeMintAddress(data.value());
      if (mint_address.has_value()) {
        // Add the contract address to the list
        discovered_mint_addresses.push_back(mint_address.value());
      }
    }
  }

  std::move(barrier_callback).Run(discovered_mint_addresses);
}

void AssetDiscoveryManager::MergeDiscoveredSolanaAssets(
    bool triggered_by_accounts_added,
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

  auto internal_callback = base::BindOnce(
      &AssetDiscoveryManager::OnGetSolanaTokenRegistry,
      weak_ptr_factory_.GetWeakPtr(), triggered_by_accounts_added,
      std::move(discovered_mint_addresses_set));

  // Fetch SOL registry tokens
  BlockchainRegistry::GetInstance()->GetAllTokens(mojom::kSolanaMainnet,
                                                  mojom::CoinType::SOL,
                                                  std::move(internal_callback));
}

void AssetDiscoveryManager::OnGetSolanaTokenRegistry(
    bool triggered_by_accounts_added,
    const base::flat_set<std::string>& discovered_mint_addresses,
    std::vector<mojom::BlockchainTokenPtr> sol_token_registry) {
  std::vector<mojom::BlockchainTokenPtr> discovered_tokens;
  for (const auto& token : sol_token_registry) {
    if (discovered_mint_addresses.contains(token->contract_address)) {
      if (!BraveWalletService::AddUserAsset(token.Clone(), prefs_)) {
        continue;
      }
      discovered_tokens.push_back(token.Clone());
    }
  }

  CompleteDiscoverAssets(std::move(discovered_tokens),
                         triggered_by_accounts_added);
}

void AssetDiscoveryManager::DiscoverEthAssets(
    const std::vector<std::string>& account_addresses,
    bool triggered_by_accounts_added) {
  if (account_addresses.empty()) {
    CompleteDiscoverAssets({}, triggered_by_accounts_added);
    return;
  }

  std::vector<mojom::BlockchainTokenPtr> user_assets =
      BraveWalletService::GetUserAssets(prefs_);
  TokenListMap token_list_map =
      BlockchainRegistry::GetInstance()->GetEthTokenListMap(
          GetAssetDiscoverySupportedEthChains());

  // Create set of all user assets per chain to use to ensure we don't
  // include assets the user has already added in the call to the BalanceScanner
  base::flat_map<std::string, base::flat_set<base::StringPiece>>
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
          base::BindOnce(&AssetDiscoveryManager::MergeDiscoveredEthAssets,
                         weak_ptr_factory_.GetWeakPtr(),
                         std::move(chain_id_to_contract_address_to_token),
                         triggered_by_accounts_added));

  // For each account address, call GetERC20TokenBalances for each chain ID
  for (const auto& account_address : account_addresses) {
    for (const auto& [chain_id, contract_addresses] :
         chain_id_to_contract_addresses) {
      auto internal_callback = base::BindOnce(
          &AssetDiscoveryManager::OnGetERC20TokenBalances,
          weak_ptr_factory_.GetWeakPtr(), barrier_callback, chain_id,
          triggered_by_accounts_added, contract_addresses);
      json_rpc_service_->GetERC20TokenBalances(contract_addresses,
                                               account_address, chain_id,
                                               std::move(internal_callback));
    }
  }
}

void AssetDiscoveryManager::OnGetERC20TokenBalances(
    base::OnceCallback<void(std::map<std::string, std::vector<std::string>>)>
        barrier_callback,
    const std::string& chain_id,
    bool triggered_by_accounts_added,
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
      HexValueToUint256(balance_results[i]->balance.value(), &balance_uint);
      if (balance_uint > 0) {
        chain_id_to_contract_addresses_with_balance[chain_id].push_back(
            contract_addresses[i]);
      }
    }
  }

  std::move(barrier_callback).Run(chain_id_to_contract_addresses_with_balance);
}

void AssetDiscoveryManager::MergeDiscoveredEthAssets(
    base::flat_map<std::string,
                   base::flat_map<std::string, mojom::BlockchainTokenPtr>>
        chain_id_to_contract_address_to_token,
    bool triggered_by_accounts_added,
    const std::vector<std::map<std::string, std::vector<std::string>>>&
        discovered_assets_results) {
  // Create a vector of BlockchainTokenPtrs to return
  std::vector<mojom::BlockchainTokenPtr> discovered_tokens;

  // Keep track of which contract addresses have been seen per chain
  base::flat_map<std::string, base::flat_set<base::StringPiece>>
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
        if (token && BraveWalletService::AddUserAsset(token.Clone(), prefs_)) {
          discovered_tokens.push_back(std::move(token));
        }
      }
    }
  }

  CompleteDiscoverAssets(std::move(discovered_tokens),
                         triggered_by_accounts_added);
}

// Called when asset discovery has completed for
void AssetDiscoveryManager::CompleteDiscoverAssets(
    std::vector<mojom::BlockchainTokenPtr> discovered_assets_for_bucket,
    bool triggered_by_accounts_added) {
  if (discover_assets_completed_callback_for_testing_) {
    std::vector<mojom::BlockchainTokenPtr> discovered_assets_for_bucket_clone;
    for (const auto& asset : discovered_assets_for_bucket) {
      discovered_assets_for_bucket_clone.push_back(asset.Clone());
    }
    discover_assets_completed_callback_for_testing_.Run(
        std::move(discovered_assets_for_bucket_clone));
  }

  // Do not emit event or modify remaining_chains_ count if
  // call was triggered by an AccountsAdded event
  if (triggered_by_accounts_added) {
    return;
  }

  // Complete the call by decrementing remaining_chains_, storing the discovered
  // assets for later, and emitting the event if this was the final chain to
  // finish
  remaining_buckets_--;
  for (auto& asset : discovered_assets_for_bucket) {
    discovered_assets_.push_back(std::move(asset));
  }

  if (remaining_buckets_ == 0) {
    wallet_service_->OnDiscoverAssetsCompleted(std::move(discovered_assets_));
    discovered_assets_.clear();
  }
}

void AssetDiscoveryManager::DiscoverAssetsOnAllSupportedChainsAccountsAdded(
    mojom::CoinType coin,
    const std::vector<std::string>& account_addresses) {
  if (coin == mojom::CoinType::ETH) {
    DiscoverEthAssets(account_addresses, true);
  } else if (coin == mojom::CoinType::SOL) {
    DiscoverSolAssets(account_addresses, true);
  }
}

void AssetDiscoveryManager::DiscoverAssetsOnAllSupportedChainsRefresh(
    std::map<mojom::CoinType, std::vector<std::string>>& account_addresses) {
  // Simple client side rate limiting (only applies to refreshes)
  const base::Time assets_last_discovered_at =
      prefs_->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  if (!assets_last_discovered_at.is_null() &&
      ((base::Time::Now() - base::Minutes(kAssetDiscoveryMinutesPerRequest)) <
       assets_last_discovered_at)) {
    wallet_service_->OnDiscoverAssetsCompleted({});
    return;
  }
  prefs_->SetTime(kBraveWalletLastDiscoveredAssetsAt, base::Time::Now());

  // Return early and do not send a notification
  // if a discover assets process is flight already
  if (remaining_buckets_ != 0) {
    return;
  }

  remaining_buckets_ = 2;  // 1 for ETH + 1 for SOL
  DiscoverSolAssets(account_addresses[mojom::CoinType::SOL], false);
  DiscoverEthAssets(account_addresses[mojom::CoinType::ETH], false);
}

void AssetDiscoveryManager::AccountsAdded(
    mojom::CoinType coin,
    const std::vector<std::string>& addresses) {
  if (!(coin == mojom::CoinType::ETH || coin == mojom::CoinType::SOL) ||
      addresses.size() == 0u) {
    return;
  }
  DiscoverAssetsOnAllSupportedChainsAccountsAdded(coin, addresses);
}

// static
// Parses the Account object for the `mint` field which is a 32 byte public key.
// See
// https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L86-L105
absl::optional<SolanaAddress> AssetDiscoveryManager::DecodeMintAddress(
    const std::vector<uint8_t>& data) {
  if (data.size() < 32) {
    return absl::nullopt;
  }

  std::vector<uint8_t> pub_key_bytes(data.begin(), data.begin() + 32);
  return SolanaAddress::FromBytes(pub_key_bytes);
}

}  // namespace brave_wallet
