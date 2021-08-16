/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/signin/public/identity_manager/identity_manager.h"

#include <utility>
#include <vector>

#define GetAccountsInCookieJar GetAccountsInCookieJar_Unused
#include "../../../../../../components/signin/public/identity_manager/identity_manager.cc"
#undef GetAccountsInCookieJar

namespace signin {

AccountsInCookieJarInfo IdentityManager::GetAccountsInCookieJar() const {
  // accounts_in_cookie_jar_info.accounts_are_fresh must be false,
  // see `SyncServiceImpl::OnEngineInitialized`
  return AccountsInCookieJarInfo(false, std::vector<gaia::ListedAccount>(),
                                 std::vector<gaia::ListedAccount>());
}

}  // namespace signin
