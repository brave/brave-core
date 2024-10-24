/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/signin/public/identity_manager/identity_manager.h"

#include <utility>
#include <vector>

#define GetAccountsInCookieJar GetAccountsInCookieJar_Unused
#define PrepareForAddingNewAccount PrepareForAddingNewAccount_Unused
#include "src/components/signin/public/identity_manager/identity_manager.cc"
#undef PrepareForAddingNewAccount
#undef GetAccountsInCookieJar

namespace signin {

AccountsInCookieJarInfo IdentityManager::GetAccountsInCookieJar() const {
  // accounts_in_cookie_jar_info.accounts_are_fresh must be false,
  // see `SyncServiceImpl::OnEngineInitialized`
  return AccountsInCookieJarInfo(/*accounts_are_fresh=*/false,
                                 std::vector<gaia::ListedAccount>());
}

void IdentityManager::PrepareForAddingNewAccount() {}

}  // namespace signin
