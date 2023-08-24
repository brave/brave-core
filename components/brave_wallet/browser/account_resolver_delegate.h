/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ACCOUNT_RESOLVER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ACCOUNT_RESOLVER_DELEGATE_H_

#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

// Wrapper around KeyringService for searching and validating accounts.
class AccountResolverDelegate {
 public:
  virtual ~AccountResolverDelegate() = default;

  // Searches for an existing account by persisted unique_key or address.
  virtual mojom::AccountIdPtr ResolveAccountId(
      const std::string* from_account_id,
      const std::string* from_address) = 0;

  // Returns true if there is an existing account equal to account_id.
  virtual bool ValidateAccountId(const mojom::AccountIdPtr& account_id) = 0;
};

}  // namespace brave_wallet
   //
#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ACCOUNT_RESOLVER_DELEGATE_H_
