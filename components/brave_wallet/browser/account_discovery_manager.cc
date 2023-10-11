// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/account_discovery_manager.h"

#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {
namespace {
const int kDiscoveryAttempts = 20;
}

AccountDiscoveryManager::DiscoveryContext::DiscoveryContext(
    const mojom::CoinType& coin_type,
    const mojom::KeyringId& keyring_id,
    const std::string& chain_id,
    size_t discovery_account_index,
    int attempts_left)
    : coin_type(coin_type),
      keyring_id(keyring_id),
      chain_id(chain_id),
      discovery_account_index(discovery_account_index),
      attempts_left(attempts_left) {}

AccountDiscoveryManager::DiscoveryContext::~DiscoveryContext() = default;

AccountDiscoveryManager::AccountDiscoveryManager(
    JsonRpcService* rpc_service,
    KeyringService* keyring_service)
    : json_rpc_service_(rpc_service), keyring_service_(keyring_service) {}

void AccountDiscoveryManager::StartDiscovery() {
  auto derived_count = GetDerivedAccountsCount();

  AddDiscoveryAccount(std::make_unique<DiscoveryContext>(
      mojom::CoinType::ETH, mojom::KeyringId::kDefault, mojom::kMainnetChainId,
      derived_count[mojom::KeyringId::kDefault], kDiscoveryAttempts));
  if (IsFilecoinEnabled()) {
    AddDiscoveryAccount(std::make_unique<DiscoveryContext>(
        mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
        mojom::kFilecoinMainnet, derived_count[mojom::KeyringId::kFilecoin],
        kDiscoveryAttempts));
  }
  if (IsSolanaEnabled()) {
    AddDiscoveryAccount(std::make_unique<DiscoveryContext>(
        mojom::CoinType::SOL, mojom::KeyringId::kSolana, mojom::kSolanaMainnet,
        derived_count[mojom::KeyringId::kSolana], kDiscoveryAttempts));
  }
}

AccountDiscoveryManager::~AccountDiscoveryManager() = default;

std::map<mojom::KeyringId, uint32_t>
AccountDiscoveryManager::GetDerivedAccountsCount() {
  std::map<mojom::KeyringId, uint32_t> derived_count;
  for (auto& acc : keyring_service_->GetAllAccountInfos()) {
    if (acc->account_id->kind == mojom::AccountKind::kDerived) {
      derived_count[acc->account_id->keyring_id]++;
    }
  }

  return derived_count;
}

void AccountDiscoveryManager::AddDiscoveryAccount(
    std::unique_ptr<DiscoveryContext> context) {
  if (context->attempts_left <= 0) {
    return;
  }

  auto addr = keyring_service_->GetDiscoveryAddress(
      context->keyring_id, context->discovery_account_index);
  if (!addr) {
    NOTREACHED();
    return;
  }

  auto chain_id = context->chain_id;
  auto coin_type = context->coin_type;
  if (context->coin_type == mojom::CoinType::ETH) {
    json_rpc_service_->GetEthTransactionCount(
        chain_id, addr.value(),
        base::BindOnce(&AccountDiscoveryManager::OnEthGetTransactionCount,
                       weak_ptr_factory_.GetWeakPtr(), std::move(context)));
  } else if (context->coin_type == mojom::CoinType::SOL) {
    // We use balance for Solana account discovery since pratically
    // getSignaturesForAddress method could work not properly sometimes when
    // node losts bigtable connection
    json_rpc_service_->GetSolanaBalance(
        addr.value(), chain_id,
        base::BindOnce(&AccountDiscoveryManager::OnResolveSolanaAccountBalance,
                       weak_ptr_factory_.GetWeakPtr(), std::move(context)));
  } else if (context->coin_type == mojom::CoinType::FIL) {
    // We use balance for Filecoin account discovery since proper method is
    // limited https://github.com/filecoin-project/lotus/issues/9728
    json_rpc_service_->GetBalance(
        addr.value(), coin_type, chain_id,
        base::BindOnce(&AccountDiscoveryManager::OnResolveAccountBalance,
                       weak_ptr_factory_.GetWeakPtr(), std::move(context)));

  } else {
    NOTREACHED();
  }
}

void AccountDiscoveryManager::OnResolveAccountBalance(
    std::unique_ptr<DiscoveryContext> context,
    const std::string& value,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    return;
  }
  ProcessDiscoveryResult(std::move(context), value != "0");
}

void AccountDiscoveryManager::OnResolveSolanaAccountBalance(
    std::unique_ptr<DiscoveryContext> context,
    uint64_t value,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess) {
    return;
  }
  ProcessDiscoveryResult(std::move(context), value > 0);
}

void AccountDiscoveryManager::OnEthGetTransactionCount(
    std::unique_ptr<DiscoveryContext> context,
    uint256_t result,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    return;
  }
  ProcessDiscoveryResult(std::move(context), result > 0);
}

void AccountDiscoveryManager::ProcessDiscoveryResult(
    std::unique_ptr<DiscoveryContext> context,
    bool result) {
  if (result) {
    auto derived_count = GetDerivedAccountsCount();

    auto last_account_index = derived_count[context->keyring_id];
    if (context->discovery_account_index + 1 > last_account_index) {
      keyring_service_->AddAccountsWithDefaultName(
          context->coin_type, context->keyring_id,
          context->discovery_account_index - last_account_index + 1);
    }

    context->discovery_account_index++;
    context->attempts_left = kDiscoveryAttempts;
    AddDiscoveryAccount(std::move(context));
  } else {
    context->discovery_account_index++;
    context->attempts_left--;
    AddDiscoveryAccount(std::move(context));
  }
}

}  // namespace brave_wallet
