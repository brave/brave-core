// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/account_discovery_manager.h"

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"

namespace brave_wallet {

namespace {
const int kDiscoveryAttempts = 20;

std::string DiscoveredBitcoinAccountName(mojom::KeyringId keyring_id,
                                         uint32_t account_index) {
  if (IsBitcoinTestnetKeyring(keyring_id)) {
    return base::StrCat(
        {"Bitcoin Testnet Account ", base::NumberToString(account_index + 1)});
  } else {
    return base::StrCat(
        {"Bitcoin Account ", base::NumberToString(account_index + 1)});
  }
}

}  // namespace

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
    JsonRpcService& rpc_service,
    KeyringService& keyring_service,
    BitcoinWalletService* bitcoin_wallet_service)
    : json_rpc_service_(rpc_service),
      keyring_service_(keyring_service),
      bitcoin_wallet_service_(bitcoin_wallet_service) {}

void AccountDiscoveryManager::StartDiscovery() {
  auto derived_count = GetDerivedAccountsCount();

  AddDiscoveryAccount(std::make_unique<DiscoveryContext>(
      mojom::CoinType::ETH, mojom::KeyringId::kDefault, mojom::kMainnetChainId,
      derived_count[mojom::KeyringId::kDefault], kDiscoveryAttempts));
  AddDiscoveryAccount(std::make_unique<DiscoveryContext>(
      mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
      mojom::kFilecoinMainnet, derived_count[mojom::KeyringId::kFilecoin],
      kDiscoveryAttempts));
  AddDiscoveryAccount(std::make_unique<DiscoveryContext>(
      mojom::CoinType::SOL, mojom::KeyringId::kSolana, mojom::kSolanaMainnet,
      derived_count[mojom::KeyringId::kSolana], kDiscoveryAttempts));

  if (IsBitcoinEnabled()) {
    if (!bitcoin_wallet_service_) {
      CHECK_IS_TEST();
    } else {
      DiscoverBitcoinAccount(mojom::KeyringId::kBitcoin84, 0);
      if (features::kBitcoinTestnetDiscovery.Get()) {
        DiscoverBitcoinAccount(mojom::KeyringId::kBitcoin84Testnet, 0);
      }
    }
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
    // We use balance for Solana account discovery since practically
    // getSignaturesForAddress method sometimes does not work properly when
    // node loses bigtable connection.
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
    NOTREACHED() << context->coin_type;
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

void AccountDiscoveryManager::DiscoverBitcoinAccount(
    mojom::KeyringId keyring_id,
    uint32_t account_index) {
  bitcoin_wallet_service_->DiscoverWalletAccount(
      keyring_id, account_index,
      base::BindOnce(&AccountDiscoveryManager::OnBitcoinDiscoverAccountsDone,
                     weak_ptr_factory_.GetWeakPtr(), keyring_id,
                     account_index));
}

void AccountDiscoveryManager::OnBitcoinDiscoverAccountsDone(
    mojom::KeyringId keyring_id,
    uint32_t account_index,
    base::expected<DiscoveredBitcoinAccount, std::string> discovered_account) {
  if (!discovered_account.has_value()) {
    return;
  }

  auto& acc = discovered_account.value();
  if (acc.next_unused_receive_index == 0 && acc.next_unused_change_index == 0) {
    // This account has no transacted addresses in blockchain. Don't add it and
    // stop discovery.
    return;
  }

  CHECK(IsBitcoinKeyring(keyring_id));

  mojom::AccountIdPtr last_bitcoin_account;
  mojom::AccountIdPtr bitcoin_account_to_update;
  for (const auto& account : keyring_service_->GetAllAccountInfos()) {
    const auto& account_id = account->account_id;
    if (account_id->coin == mojom::CoinType::BTC &&
        account_id->keyring_id == keyring_id) {
      if (account->account_id->account_index == account_index) {
        bitcoin_account_to_update = account->account_id->Clone();
      }

      if (!last_bitcoin_account ||
          last_bitcoin_account->account_index < account_id->account_index) {
        last_bitcoin_account = account_id->Clone();
      }
    }
  }

  if (!bitcoin_account_to_update) {
    if (last_bitcoin_account &&
        last_bitcoin_account->account_index + 1 != account_index) {
      // We don't allow gaps in account indexes, so just return if discovered
      // account would not be the next account.
      return;
    }

    auto created_account = keyring_service_->AddAccountSync(
        mojom::CoinType::BTC, keyring_id,
        DiscoveredBitcoinAccountName(keyring_id, account_index));
    if (!created_account) {
      return;
    }
    bitcoin_account_to_update = created_account->account_id.Clone();
  }

  keyring_service_->UpdateNextUnusedAddressForBitcoinAccount(
      bitcoin_account_to_update, acc.next_unused_receive_index,
      acc.next_unused_change_index);

  DiscoverBitcoinAccount(keyring_id, account_index + 1);
}

}  // namespace brave_wallet
