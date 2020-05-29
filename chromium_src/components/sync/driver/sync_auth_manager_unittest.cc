/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/fake_access_token_fetcher.h"

namespace syncer {

class SyncAuthManager;

namespace {
const char sync_code1[] =
    "badge unique kiwi orient spring venue piano "
    "lake admit ill roof brother grant hour better "
    "proud cabbage fee slow economy wage final fox cancel";

const char sync_code2[] =
    "marine seminar head allow quick hold switch boost "
    "suffer sibling situate unhappy give movie steel spin "
    "dumb broccoli enter series power fog oven leisure";

const char account_id_str[] = "gaia_id_for_user_gmail.com";

brave_sync::FakeAccessTokenFetcher* CreateAccessTokenFetcher(
    SyncAuthManager* manager);
void WaitForAccessTokenResponse(brave_sync::FakeAccessTokenFetcher* fetcher);
}  // namespace

}  // namespace syncer

#define ForwardsSecondaryAccountEvents DISABLED_ForwardsSecondaryAccountEvents
#define ClearsCredentialsOnInvalidRefreshToken \
  DISABLED_ClearsCredentialsOnInvalidRefreshToken
#define RequestsAccessTokenWhenInvalidRefreshTokenResolved \
  DISABLED_RequestsAccessTokenWhenInvalidRefreshTokenResolved
#define DoesNotRequestAccessTokenIfSyncInactive \
  DISABLED_DoesNotRequestAccessTokenIfSyncInactive
#define IgnoresCookieJar DISABLED_IgnoresCookieJar
#define UsesCookieJar DISABLED_UsesCookieJar
#define DropsAccountWhenCookieGoesAway DISABLED_DropsAccountWhenCookieGoesAway
#define DropsAccountWhenRefreshTokenGoesAway \
  DISABLED_DropsAccountWhenRefreshTokenGoesAway
#define PrefersPrimaryAccountOverCookie DISABLED_PrefersPrimaryAccountOverCookie
#define OnlyUsesFirstCookieAccount DISABLED_OnlyUsesFirstCookieAccount

#define BRAVE_SIGN_IN_1                                   \
  account_id = CoreAccountId::FromString(account_id_str); \
  auth_manager->DeriveSigningKeys(sync_code1);
#define BRAVE_SIGN_IN_1_WITH_ACCESS_TOKEN_FETCHER                            \
  account_id = CoreAccountId::FromString(account_id_str);                    \
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get()); \
  auth_manager->DeriveSigningKeys(sync_code1);
#define BRAVE_SIGN_IN_1_WITH_ACCESS_TOKEN_FETCHER_UNUSED  \
  account_id = CoreAccountId::FromString(account_id_str); \
  CreateAccessTokenFetcher(auth_manager.get());           \
  auth_manager->DeriveSigningKeys(sync_code1);
#define BRAVE_SIGN_IN_2                                          \
  second_account_id = CoreAccountId::FromString(account_id_str); \
  auth_manager->DeriveSigningKeys(sync_code2);
#define BRAVE_SIGN_OUT auth_manager->ResetKeys();
#define BRAVE_WAIT_FOR_ACCESS_TOKEN \
  WaitForAccessTokenResponse(access_token_fetcher);
#define BRAVE_ON_REFRESH_TOKEN_REMOVED \
  auth_manager->OnRefreshTokenRemovedForAccount(account_id);
#define BRAVE_SET_ACCESS_TOKEN_RESPONSE                                        \
  access_token_fetcher->SetAccessTokenResponseForTest(                         \
      brave_sync::AccessTokenConsumer::TokenResponse(                          \
          "access_token_2", base::Time::Now() + base::TimeDelta::FromHours(1)));
#define BRAVE_ON_REFRESH_TOKEN_UPDATED  \
  CoreAccountInfo account_info;         \
  account_info.account_id = account_id; \
  auth_manager->OnRefreshTokenUpdatedForAccount(account_info);
#define BRAVE_SET_ACCESS_TOKEN_RESPONSE_CALLBACK \
  access_token_fetcher->SetTokenResponseCallback(access_token_requested.Get());

#define BRAVE_FORWARDS_CREDENTIALS_EVENTS \
  BRAVE_SET_ACCESS_TOKEN_RESPONSE         \
  BRAVE_ON_REFRESH_TOKEN_UPDATED

#define BRAVE_RETRIES_ACCESS_TOKEN_FETCH_WITH_BACKOFF_ON_TRANSIENT_FAILURE \
  BRAVE_SIGN_IN_1_WITH_ACCESS_TOKEN_FETCHER                                \
  access_token_fetcher->SetTokenResponseError(                             \
      GoogleServiceAuthError::FromConnectionError(net::ERR_TIMED_OUT));

#define BRAVE_ABORTS_ACCESS_TOKEN_FETCH_ON_PERSISTENT_FAILURE    \
  BRAVE_SIGN_IN_1_WITH_ACCESS_TOKEN_FETCHER                      \
  GoogleServiceAuthError auth_error_1 =                          \
      GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(  \
          GoogleServiceAuthError::InvalidGaiaCredentialsReason:: \
              CREDENTIALS_REJECTED_BY_SERVER);                   \
  access_token_fetcher->SetTokenResponseError(auth_error_1);

#define BRAVE_REQUESTS_NEW_ACCESS_TOKEN_ON_EXPIRY \
  BRAVE_SET_ACCESS_TOKEN_RESPONSE                 \
  BRAVE_WAIT_FOR_ACCESS_TOKEN

#define BRAVE_REQUESTS_NEW_ACCESS_TOKEN_ON_REFRESH_TOKEN_UPDATE \
  BRAVE_SET_ACCESS_TOKEN_RESPONSE                               \
  BRAVE_ON_REFRESH_TOKEN_UPDATED

#define BRAVE_DOES_NOT_REQUEST_ACCESS_TOKEN_AUTONOMOUSLY \
  BRAVE_SET_ACCESS_TOKEN_RESPONSE_CALLBACK               \
  BRAVE_ON_REFRESH_TOKEN_UPDATED

#define BRAVE_CLEARS_CREDENTIALS_ON_REFRESH_TOKEN_REMOVAL \
  BRAVE_SET_ACCESS_TOKEN_RESPONSE_CALLBACK                \
  BRAVE_ON_REFRESH_TOKEN_REMOVED

#include "../../../../../components/sync/driver/sync_auth_manager_unittest.cc"
#undef ForwardsSecondaryAccountEvents
#undef ClearsCredentialsOnInvalidRefreshToken
#undef RequestsAccessTokenWhenInvalidRefreshTokenResolved
#undef DoesNotRequestAccessTokenIfSyncInactive
#undef IgnoresCookieJar
#undef UsesCookieJar
#undef DropsAccountWhenCookieGoesAway
#undef DropsAccountWhenRefreshTokenGoesAway
#undef PrefersPrimaryAccountOverCookie
#undef OnlyUsesFirstCookieAccount

#undef BRAVE_SIGN_IN_1
#undef BRAVE_SIGN_IN_1_WITH_ACCESS_TOKEN_FETCHER
#undef BRAVE_SIGN_IN_1_WITH_ACCESS_TOKEN_FETCHER_UNUSED
#undef BRAVE_SIGN_IN_2
#undef BRAVE_SIGN_OUT
#undef BRAVE_WAIT_FOR_ACCESS_TOKEN
#undef BRAVE_ON_REFRESH_TOKEN_REMOVED
#undef BRAVE_SET_ACCESS_TOKEN_RESPONSE
#undef BRAVE_ON_REFRESH_TOKEN_UPDATED
#undef BRAVE_SET_ACCESS_TOKEN_RESPONSE_CALLBACK

#undef BRAVE_FORWARDS_CREDENTIALS_EVENTS
#undef BRAVE_RETRIES_ACCESS_TOKEN_FETCH_WITH_BACKOFF_ON_TRANSIENT_FAILURE
#undef BRAVE_ABORTS_ACCESS_TOKEN_FETCH_ON_PERSISTENT_FAILURE
#undef BRAVE_REQUESTS_NEW_ACCESS_TOKEN_ON_EXPIRY
#undef BRAVE_REQUESTS_NEW_ACCESS_TOKEN_ON_REFRESH_TOKEN_UPDATE
#undef BRAVE_DOES_NOT_REQUEST_ACCESS_TOKEN_AUTONOMOUSLY
#undef BRAVE_CLEARS_CREDENTIALS_ON_REFRESH_TOKEN_REMOVAL

namespace syncer {

namespace {
brave_sync::FakeAccessTokenFetcher* CreateAccessTokenFetcher(
    SyncAuthManager* manager) {
  manager->SetAccessTokenFetcherForTest(
      std::make_unique<brave_sync::FakeAccessTokenFetcher>(manager));
  return static_cast<brave_sync::FakeAccessTokenFetcher*>(
      manager->GetAccessTokenFetcherForTest());
}

void WaitForAccessTokenResponse(brave_sync::FakeAccessTokenFetcher* fetcher) {
  base::RunLoop run_loop;
  fetcher->SetTokenResponseCallback(run_loop.QuitClosure());
  run_loop.Run();
}

}  // namespace

}  // namespace syncer
