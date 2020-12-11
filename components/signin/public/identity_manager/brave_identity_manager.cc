/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/signin/public/identity_manager/brave_identity_manager.h"

#include <utility>
#include <vector>

#include "components/signin/public/base/signin_client.h"
#include "components/signin/public/identity_manager/accounts_in_cookie_jar_info.h"
#include "components/signin/public/identity_manager/identity_manager.h"

namespace signin {

BraveIdentityManager::BraveIdentityManager(
    IdentityManager::InitParameters&& parameters)
    : IdentityManager(std::move(parameters)) {}

BraveIdentityManager::~BraveIdentityManager() {}

AccountsInCookieJarInfo BraveIdentityManager::GetAccountsInCookieJar() const {
  // accounts_in_cookie_jar_info.accounts_are_fresh must be false,
  // see `ProfileSyncService::OnEngineInitialized`
  return AccountsInCookieJarInfo(false, std::vector<gaia::ListedAccount>(),
                                 std::vector<gaia::ListedAccount>());
}

}  // namespace signin
