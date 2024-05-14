/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SIGNIN_INTERNAL_IDENTITY_MANAGER_BRAVE_PRIMARY_ACCOUNT_MUTATOR_IMPL_H_
#define BRAVE_COMPONENTS_SIGNIN_INTERNAL_IDENTITY_MANAGER_BRAVE_PRIMARY_ACCOUNT_MUTATOR_IMPL_H_

#include "components/signin/internal/identity_manager/primary_account_mutator_impl.h"

namespace signin {

class BravePrimaryAccountMutatorImpl : public PrimaryAccountMutatorImpl {
 public:
  BravePrimaryAccountMutatorImpl(AccountTrackerService* account_tracker,
                                 PrimaryAccountManager* primary_account_manager,
                                 PrefService* pref_service,
                                 SigninClient* signin_client);
  ~BravePrimaryAccountMutatorImpl() override;

#if !BUILDFLAG(IS_CHROMEOS_ASH)
  bool ClearPrimaryAccount(
      signin_metrics::ProfileSignout source_metric) override;
#endif
};

}  //  namespace signin

#endif  // BRAVE_COMPONENTS_SIGNIN_INTERNAL_IDENTITY_MANAGER_BRAVE_PRIMARY_ACCOUNT_MUTATOR_IMPL_H_
