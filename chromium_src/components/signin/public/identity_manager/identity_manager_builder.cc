/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../components/signin/public/identity_manager/identity_manager_builder.cc"

#include "brave/components/signin/internal/identity_manager/brave_primary_account_mutator_impl.h"
#include "brave/components/signin/public/identity_manager/brave_identity_manager.h"

namespace signin {

namespace {

IdentityManager::InitParameters BuildBraveIdentityManagerInitParameters(
    IdentityManagerBuildParams* params) {
  IdentityManager::InitParameters init_params =
      BuildIdentityManagerInitParameters(params);

  init_params.primary_account_mutator =
      std::make_unique<BravePrimaryAccountMutatorImpl>(
          init_params.account_tracker_service.get(),
          init_params.token_service.get(),
          init_params.primary_account_manager.get(),
          params->pref_service, params->account_consistency);

  return init_params;
}

}  // namespace

std::unique_ptr<BraveIdentityManager> BuildBraveIdentityManager(
    IdentityManagerBuildParams* params) {
  return std::make_unique<BraveIdentityManager>(
      BuildBraveIdentityManagerInitParameters(params));
}

}  // namespace signin
