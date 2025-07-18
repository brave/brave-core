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
        IsCardanoMainnetKeyring(account->account_id->keyring_id)) {
      ids.push_back(GetAccountPermissionIdentifier(account->account_id));
    }
  }
  return ids;
}

mojom::AccountIdPtr GetCardanoAllowedSelectedAccount(
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

  std::vector<mojom::AccountIdPtr> filtered_accounts;

  for (const auto& account : keyring_service->GetAllAccountInfos()) {
    bool is_account_allowed = base::Contains(
        *allowed_accounts, GetAccountPermissionIdentifier(account->account_id));
    bool is_selected_account = account->account_id->unique_key ==
                               selected_account->account_id->unique_key;
    if (is_account_allowed) {
      if (is_selected_account) {
        filtered_accounts.clear();
        filtered_accounts.push_back(account->account_id.Clone());
        break;
      } else {
        filtered_accounts.push_back(account->account_id.Clone());
      }
    }
  }
  return filtered_accounts.size() > 0 ? std::move(filtered_accounts[0])
                                      : nullptr;
}

}  // namespace brave_wallet
