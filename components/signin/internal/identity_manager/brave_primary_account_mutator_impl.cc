/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/signin/internal/identity_manager/brave_primary_account_mutator_impl.h"

class AccountTrackerService;
class PrefService;
class PrimaryAccountManager;

namespace signin {

BravePrimaryAccountMutatorImpl::BravePrimaryAccountMutatorImpl(
    AccountTrackerService* account_tracker,
    PrimaryAccountManager* primary_account_manager,
    PrefService* pref_service)
    : PrimaryAccountMutatorImpl(account_tracker,
                                primary_account_manager,
                                pref_service) {}

BravePrimaryAccountMutatorImpl::~BravePrimaryAccountMutatorImpl() = default;

#if !defined(OS_CHROMEOS)
bool BravePrimaryAccountMutatorImpl::ClearPrimaryAccount(
    ClearAccountsAction action,
    signin_metrics::ProfileSignout source_metric,
    signin_metrics::SignoutDelete delete_metric) {
  return true;
}
#endif

}  //  namespace signin
