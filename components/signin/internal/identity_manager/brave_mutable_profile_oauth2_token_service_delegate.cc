/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/signin/internal/identity_manager/brave_mutable_profile_oauth2_token_service_delegate.h"

#include <memory>
#include <utility>

#include "components/signin/public/webdata/token_web_data.h"

#if BUILDFLAG(ENABLE_BOUND_SESSION_CREDENTIALS)
#include "components/signin/internal/identity_manager/token_binding_helper.h"
#endif  // BUILDFLAG(ENABLE_BOUND_SESSION_CREDENTIALS)

BraveMutableProfileOAuth2TokenServiceDelegate::
    BraveMutableProfileOAuth2TokenServiceDelegate(
        SigninClient* client,
        AccountTrackerService* account_tracker_service,
        network::NetworkConnectionTracker* network_connection_tracker,
        scoped_refptr<TokenWebData> token_web_data,
        signin::AccountConsistencyMethod account_consistency,
        RevokeAllTokensOnLoad revoke_all_tokens_on_load,
#if BUILDFLAG(ENABLE_BOUND_SESSION_CREDENTIALS)
        std::unique_ptr<TokenBindingHelper> token_binding_helper,
#endif  // BUILDFLAG(ENABLE_BOUND_SESSION_CREDENTIALS)
        FixRequestErrorCallback fix_request_error_callback)
    : MutableProfileOAuth2TokenServiceDelegate(client,
                                               account_tracker_service,
                                               network_connection_tracker,
                                               token_web_data,
                                               account_consistency,
                                               revoke_all_tokens_on_load,
#if BUILDFLAG(ENABLE_BOUND_SESSION_CREDENTIALS)
                                               std::move(token_binding_helper),
#endif  // BUILDFLAG(ENABLE_BOUND_SESSION_CREDENTIALS)
                                               fix_request_error_callback),
      account_tracker_service_(account_tracker_service) {
}

BraveMutableProfileOAuth2TokenServiceDelegate::
    ~BraveMutableProfileOAuth2TokenServiceDelegate() = default;

void BraveMutableProfileOAuth2TokenServiceDelegate::LoadCredentials(
    const CoreAccountId& primary_account_id,
    bool is_syncing) {
  if (!account_tracker_service_->GetAccounts().size())
    return;
  MutableProfileOAuth2TokenServiceDelegate::LoadCredentials(primary_account_id,
                                                            is_syncing);
}
