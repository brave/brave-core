/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_dapp_utils.h"

#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

std::vector<std::string> GetCardanoAccountPermissionIdentifiers(
    KeyringService* keyring_service) {
  std::vector<std::string> ids;
  auto accounts = keyring_service->GetAllAccountsSync();
  for (const auto& account : accounts->accounts) {
    if (account && account->account_id &&
        IsCardanoKeyring(account->account_id->keyring_id)) {
      ids.push_back(GetAccountPermissionIdentifier(account->account_id));
    }
  }
  return ids;
}

mojom::AccountIdPtr GetCardanoPreferredDappAccount(
    BraveWalletProviderDelegate* delegate,
    KeyringService* keyring_service) {
  auto cardano_account_ids =
      GetCardanoAccountPermissionIdentifiers(keyring_service);

  if (cardano_account_ids.empty()) {
    return nullptr;
  }

  const auto allowed_accounts =
      delegate->GetAllowedAccounts(mojom::CoinType::ADA, cardano_account_ids);

  if (!allowed_accounts || allowed_accounts->empty()) {
    return nullptr;
  }

  auto selected_account = keyring_service->GetSelectedCardanoDappAccount();
  bool is_selected_account_allowed =
      selected_account &&
      base::Contains(*allowed_accounts, GetAccountPermissionIdentifier(
                                            selected_account->account_id));
  if (is_selected_account_allowed) {
    return selected_account->account_id.Clone();
  }

  // Since there is no account selection when permissions are granted,
  // we use first allowed account.
  // Similar behavior implemented in EthereumProviderImpl.
  for (const auto& account : keyring_service->GetAllAccountInfos()) {
    bool is_account_allowed = base::Contains(
        *allowed_accounts, GetAccountPermissionIdentifier(account->account_id));
    if (is_account_allowed) {
      return account->account_id.Clone();
    }
  }
  return nullptr;
}

}  // namespace brave_wallet
