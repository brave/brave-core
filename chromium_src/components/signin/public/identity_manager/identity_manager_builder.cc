/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../components/signin/public/identity_manager/identity_manager_builder.cc"

#include "brave/components/signin/internal/identity_manager/brave_primary_account_mutator_impl.h"
#include "brave/components/signin/public/identity_manager/brave_identity_manager.h"
#include "brave/components/signin/public/identity_manager/brave_identity_manager_builder.h"

namespace signin {

IdentityManager::InitParameters BuildBraveIdentityManagerInitParameters(
    IdentityManagerBuildParams* params) {
  std::unique_ptr<AccountTrackerService> account_tracker_service =
      BuildAccountTrackerService(params->pref_service, params->profile_path);

  std::unique_ptr<ProfileOAuth2TokenService> token_service =
      BuildProfileOAuth2TokenService(
          params->pref_service, account_tracker_service.get(),
          params->network_connection_tracker, params->account_consistency,
#if defined(OS_CHROMEOS)
          params->account_manager, params->is_regular_profile,
#endif
#if !defined(OS_ANDROID)
          params->delete_signin_cookies_on_exit, params->token_web_data,
#endif
#if defined(OS_IOS)
          std::move(params->device_accounts_provider),
#endif
#if defined(OS_WIN)
          params->reauth_callback,
#endif
          params->signin_client);

  auto gaia_cookie_manager_service = std::make_unique<GaiaCookieManagerService>(
      token_service.get(), params->signin_client);

  std::unique_ptr<PrimaryAccountManager> primary_account_manager =
      BuildPrimaryAccountManager(params->signin_client,
                                 params->account_consistency,
                                 account_tracker_service.get(),
                                 token_service.get(), params->local_state);

  IdentityManager::InitParameters init_params;

  init_params.primary_account_mutator =
      std::make_unique<BravePrimaryAccountMutatorImpl>(
          account_tracker_service.get(), primary_account_manager.get(),
          params->pref_service);

  init_params.accounts_mutator =
      BuildAccountsMutator(params->pref_service, account_tracker_service.get(),
                           token_service.get(), primary_account_manager.get());

  init_params.accounts_cookie_mutator =
      std::make_unique<AccountsCookieMutatorImpl>(
          params->signin_client, token_service.get(),
          gaia_cookie_manager_service.get(), account_tracker_service.get());

  init_params.diagnostics_provider = std::make_unique<DiagnosticsProviderImpl>(
      token_service.get(), gaia_cookie_manager_service.get());

  init_params.account_fetcher_service = BuildAccountFetcherService(
      params->signin_client, token_service.get(), account_tracker_service.get(),
      std::move(params->image_decoder));

#if defined(OS_IOS) || defined(OS_ANDROID)
  init_params.device_accounts_synchronizer =
      std::make_unique<DeviceAccountsSynchronizerImpl>(
          token_service->GetDelegate());
#endif

  init_params.account_tracker_service = std::move(account_tracker_service);
  init_params.gaia_cookie_manager_service =
      std::move(gaia_cookie_manager_service);
  init_params.primary_account_manager = std::move(primary_account_manager);
  init_params.token_service = std::move(token_service);
#if defined(OS_CHROMEOS)
  init_params.chromeos_account_manager = params->account_manager;
#endif

  return init_params;
}

std::unique_ptr<BraveIdentityManager> BuildBraveIdentityManager(
    IdentityManagerBuildParams* params) {
  return std::make_unique<BraveIdentityManager>(
      BuildBraveIdentityManagerInitParameters(params));
}

}  // namespace signin
