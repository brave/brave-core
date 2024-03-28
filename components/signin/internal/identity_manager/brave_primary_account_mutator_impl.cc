/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/signin/internal/identity_manager/brave_primary_account_mutator_impl.h"

#include "components/signin/public/base/account_consistency_method.h"

class AccountTrackerService;
class PrefService;
class PrimaryAccountManager;
class ProfileOAuth2TokenService;

namespace signin {

BravePrimaryAccountMutatorImpl::BravePrimaryAccountMutatorImpl(
    AccountTrackerService* account_tracker,
    PrimaryAccountManager* primary_account_manager,
    PrefService* pref_service,
    SigninClient* signin_client)
    : PrimaryAccountMutatorImpl(account_tracker,
                                primary_account_manager,
                                pref_service,
                                signin_client) {}

BravePrimaryAccountMutatorImpl::~BravePrimaryAccountMutatorImpl() = default;

#if !BUILDFLAG(IS_CHROMEOS)
bool BravePrimaryAccountMutatorImpl::ClearPrimaryAccount(
    signin_metrics::ProfileSignout source_metric) {
  return true;
}
#endif

}  //  namespace signin
