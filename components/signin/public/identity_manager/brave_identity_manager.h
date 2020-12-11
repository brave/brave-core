/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIGNIN_PUBLIC_IDENTITY_MANAGER_BRAVE_IDENTITY_MANAGER_H_
#define BRAVE_COMPONENTS_SIGNIN_PUBLIC_IDENTITY_MANAGER_BRAVE_IDENTITY_MANAGER_H_

#include <memory>

#include "components/signin/public/identity_manager/identity_manager.h"

// Purpose of this class is to redirect to empty implementation
// syncer::ProfileSyncService::identity_manager_
// These must be overridden and empty:
//  GetAccountsInCookieJar
//  GetPrimaryAccountMutator - done in `BuildBraveIdentityManagerInitParameters`

class GoogleServiceAuthError;
class BraveAccountFetcherService;

namespace signin {

struct AccountsInCookieJarInfo;
class PrimaryAccountMutator;
class BravePrimaryAccountMutator;

class BraveIdentityManager : public IdentityManager {
 public:
  explicit BraveIdentityManager(IdentityManager::InitParameters&& parameters);
  ~BraveIdentityManager() override;

  BraveIdentityManager(const BraveIdentityManager&) = delete;
  BraveIdentityManager& operator=(const BraveIdentityManager&) = delete;

  AccountsInCookieJarInfo GetAccountsInCookieJar() const override;
};

}  // namespace signin

#endif  // BRAVE_COMPONENTS_SIGNIN_PUBLIC_IDENTITY_MANAGER_BRAVE_IDENTITY_MANAGER_H_
