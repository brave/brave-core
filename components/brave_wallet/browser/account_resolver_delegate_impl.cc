/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/account_resolver_delegate_impl.h"

namespace brave_wallet {

AccountResolverDelegateImpl::AccountResolverDelegateImpl(
    KeyringService& keyring_service)
    : keyring_service_(keyring_service) {}

mojom::AccountIdPtr AccountResolverDelegateImpl::ResolveAccountId(
    const std::string* from_account_id,
    const std::string* from_address) {
  const auto& accounts = keyring_service_->GetAllAccountInfos();
  if (from_account_id) {
    for (auto& account : accounts) {
      DCHECK(!account->account_id->unique_key.empty());
      if (account->account_id->unique_key == *from_account_id) {
        return account->account_id->Clone();
      }
    }
  } else if (from_address && !from_address->empty()) {
    for (auto& account : accounts) {
      if (base::EqualsCaseInsensitiveASCII(account->address, *from_address)) {
        return account->account_id->Clone();
      }
    }
  }

  return nullptr;
}

bool AccountResolverDelegateImpl::ValidateAccountId(
    const mojom::AccountIdPtr& account_id) {
  const auto& accounts = keyring_service_->GetAllAccountInfos();
  for (auto& account : accounts) {
    if (account->account_id == account_id) {
      return true;
    }
  }
  return false;
}

}  // namespace brave_wallet
