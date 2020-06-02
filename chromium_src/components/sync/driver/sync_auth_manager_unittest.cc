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

// DISABLED and reenable with Brave specific login flow to prevent patching
// new added cases in the future wll be caught by failure
#define IgnoresEventsIfNotRegistered DISABLED_IgnoresEventsIfNotRegistered
#define ForwardsPrimaryAccountEvents DISABLED_ForwardsPrimaryAccountEvents
#define NotifiesOfSignoutBeforeAccessTokenIsGone \
  DISABLED_NotifiesOfSignoutBeforeAccessTokenIsGone
#define ClearsAuthErrorOnSignout DISABLED_ClearsAuthErrorOnSignout
#define DoesNotClearAuthErrorOnSyncDisable \
  DISABLED_DoesNotClearAuthErrorOnSyncDisable
#define ForwardsCredentialsEvents DISABLED_ForwardsCredentialsEvents
#define RequestsAccessTokenOnSyncStartup \
  DISABLED_RequestsAccessTokenOnSyncStartup
#define RetriesAccessTokenFetchWithBackoffOnTransientFailure \
  DISABLED_RetriesAccessTokenFetchWithBackoffOnTransientFailure
#define \
RetriesAccessTokenFetchWithBackoffOnFirstCancelTransientFailWhenDisabled \
DISABLED_RetriesAccessTokenFetchWithBackoffOnFirstCancelTransientFailWhenDisabled // NOLINT
#define RetriesAccessTokenFetchWithoutBackoffOnceOnFirstCancelTransientFailure \
DISABLED_RetriesAccessTokenFetchWithoutBackoffOnceOnFirstCancelTransientFailure
#define RetriesAccessTokenFetchOnFirstCancelTransientFailure \
  DISABLED_RetriesAccessTokenFetchOnFirstCancelTransientFailure
#define AbortsAccessTokenFetchOnPersistentFailure \
  DISABLED_AbortsAccessTokenFetchOnPersistentFailure
#define FetchesNewAccessTokenWithBackoffOnServerError \
  DISABLED_FetchesNewAccessTokenWithBackoffOnServerError
#define ExposesServerError DISABLED_ExposesServerError
#define ClearsServerErrorOnSyncDisable DISABLED_ClearsServerErrorOnSyncDisable
#define RequestsNewAccessTokenOnExpiry DISABLED_RequestsNewAccessTokenOnExpiry
#define RequestsNewAccessTokenOnRefreshTokenUpdate \
  DISABLED_RequestsNewAccessTokenOnRefreshTokenUpdate
#define DoesNotRequestAccessTokenAutonomously \
  DISABLED_DoesNotRequestAccessTokenAutonomously
#define ClearsCredentialsOnRefreshTokenRemoval \
  DISABLED_ClearsCredentialsOnRefreshTokenRemoval

// DISABLED cases
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

#include "../../../../../components/sync/driver/sync_auth_manager_unittest.cc"

#undef IgnoresEventsIfNotRegistered
#undef ForwardsPrimaryAccountEvents
#undef NotifiesOfSignoutBeforeAccessTokenIsGone
#undef ClearsAuthErrorOnSignout
#undef DoesNotClearAuthErrorOnSyncDisable
#undef ForwardsCredentialsEvents
#undef RequestsAccessTokenOnSyncStartup
#undef RetriesAccessTokenFetchWithBackoffOnTransientFailure
#undef RetriesAccessTokenFetchWithBackoffOnFirstCancelTransientFailWhenDisabled
#undef RetriesAccessTokenFetchWithoutBackoffOnceOnFirstCancelTransientFailure
#undef RetriesAccessTokenFetchOnFirstCancelTransientFailure
#undef AbortsAccessTokenFetchOnPersistentFailure
#undef FetchesNewAccessTokenWithBackoffOnServerError
#undef ExposesServerError
#undef ClearsServerErrorOnSyncDisable
#undef RequestsNewAccessTokenOnExpiry
#undef RequestsNewAccessTokenOnRefreshTokenUpdate
#undef DoesNotRequestAccessTokenAutonomously
#undef ClearsCredentialsOnRefreshTokenRemoval

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

TEST_F(SyncAuthManagerTest, BraveIgnoresEventsIfNotRegistered) {
  base::MockCallback<AccountStateChangedCallback> account_state_changed;
  base::MockCallback<CredentialsChangedCallback> credentials_changed;
  EXPECT_CALL(account_state_changed, Run()).Times(0);
  EXPECT_CALL(credentials_changed, Run()).Times(0);
  auto auth_manager =
      CreateAuthManager(account_state_changed.Get(), credentials_changed.Get());

  // Fire some auth events. We haven't called RegisterForAuthNotifications, so
  // none of this should result in any callback calls.
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  account_id = CoreAccountId::FromString(account_id_str);
  auth_manager->DeriveSigningKeys(sync_code1);
  // Without RegisterForAuthNotifications, the active account should always be
  // reported as empty.
  EXPECT_TRUE(
      auth_manager->GetActiveAccountInfo().account_info.account_id.empty());
  identity_env()->SetRefreshTokenForPrimaryAccount();
  EXPECT_TRUE(
      auth_manager->GetActiveAccountInfo().account_info.account_id.empty());

// ChromeOS doesn't support sign-out.
#if !defined(OS_CHROMEOS)
  identity_env()->ClearPrimaryAccount();
  auth_manager->ResetKeys();
  EXPECT_TRUE(
      auth_manager->GetActiveAccountInfo().account_info.account_id.empty());
#endif  // !defined(OS_CHROMEOS)
}

// ChromeOS doesn't support sign-out.
#if !defined(OS_CHROMEOS)
TEST_F(SyncAuthManagerTest, BraveForwardsPrimaryAccountEvents) {
  // Start out already signed in before the SyncAuthManager is created.
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;

  base::MockCallback<AccountStateChangedCallback> account_state_changed;
  base::MockCallback<CredentialsChangedCallback> credentials_changed;
  EXPECT_CALL(account_state_changed, Run()).Times(0);
  EXPECT_CALL(credentials_changed, Run()).Times(0);
  auto auth_manager =
      CreateAuthManager(account_state_changed.Get(), credentials_changed.Get());
  account_id = CoreAccountId::FromString(account_id_str);
  auth_manager->DeriveSigningKeys(sync_code1);

  auth_manager->RegisterForAuthNotifications();

  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  // Sign out of the account.
  EXPECT_CALL(account_state_changed, Run());
  // Note: The ordering of removing the refresh token and the actual sign-out is
  // undefined, see comment on IdentityManager::Observer. So we might or might
  // not get a |credentials_changed| call here.
  EXPECT_CALL(credentials_changed, Run()).Times(testing::AtMost(1));
  identity_env()->ClearPrimaryAccount();
  auth_manager->ResetKeys();
  EXPECT_TRUE(
      auth_manager->GetActiveAccountInfo().account_info.account_id.empty());

  // Sign in to a different account.
  EXPECT_CALL(account_state_changed, Run());
  CoreAccountId second_account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  second_account_id = CoreAccountId::FromString(account_id_str);
  auth_manager->DeriveSigningKeys(sync_code2);
  EXPECT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            second_account_id);
}

TEST_F(SyncAuthManagerTest, BraveNotifiesOfSignoutBeforeAccessTokenIsGone) {
  // Start out already signed in before the SyncAuthManager is created.
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;

  base::MockCallback<AccountStateChangedCallback> account_state_changed;
  base::MockCallback<CredentialsChangedCallback> credentials_changed;
  auto auth_manager =
      CreateAuthManager(account_state_changed.Get(), base::DoNothing());

  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  auth_manager->RegisterForAuthNotifications();

  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();

  // Make sure an access token is available.
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");

  // Sign out of the account.
  EXPECT_CALL(account_state_changed, Run()).WillOnce([&]() {
    // At the time the callback gets run, the access token should still be here.
    EXPECT_FALSE(auth_manager->GetCredentials().access_token.empty());
  });
  identity_env()->ClearPrimaryAccount();
  auth_manager->ResetKeys();
  // After the signout is complete, the access token should be gone.
  EXPECT_TRUE(
      auth_manager->GetActiveAccountInfo().account_info.account_id.empty());
}
#endif  // !defined(OS_CHROMEOS)

// ChromeOS doesn't support sign-out.
#if !defined(OS_CHROMEOS)
TEST_F(SyncAuthManagerTest, BraveClearsAuthErrorOnSignout) {
  // Start out already signed in before the SyncAuthManager is created.
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;

  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auth_manager->DeriveSigningKeys(sync_code1);

  auth_manager->RegisterForAuthNotifications();

  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);
  ASSERT_EQ(auth_manager->GetLastAuthError().state(),
            GoogleServiceAuthError::NONE);

  // Sign out of the account.
  // The ordering of removing the refresh token and the actual sign-out is
  // undefined, see comment on IdentityManager::Observer. Here, explicitly
  // revoke the refresh token first to force an auth error.
  identity_env()->RemoveRefreshTokenForPrimaryAccount();
  auth_manager->OnRefreshTokenRemovedForAccount(account_id);

  ASSERT_NE(auth_manager->GetLastAuthError().state(),
            GoogleServiceAuthError::NONE);

  // Now actually sign out, i.e. remove the primary account. This should clear
  // the auth error, since it's now not meaningful anymore.
  identity_env()->ClearPrimaryAccount();
  auth_manager->ResetKeys();
  EXPECT_EQ(auth_manager->GetLastAuthError().state(),
            GoogleServiceAuthError::NONE);
}
#endif  // !defined(OS_CHROMEOS)

TEST_F(SyncAuthManagerTest, BraveDoesNotClearAuthErrorOnSyncDisable) {
  // Start out already signed in before the SyncAuthManager is created.
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;

  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);

  auth_manager->RegisterForAuthNotifications();

  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);
  ASSERT_EQ(auth_manager->GetLastAuthError().state(),
            GoogleServiceAuthError::NONE);

  auth_manager->ConnectionOpened();

  // Force an auth error by revoking the refresh token.
  identity_env()->RemoveRefreshTokenForPrimaryAccount();
  auth_manager->OnRefreshTokenRemovedForAccount(account_id);
  ASSERT_NE(auth_manager->GetLastAuthError().state(),
            GoogleServiceAuthError::NONE);

  // Now Sync gets turned off, e.g. because the user disabled it.
  auth_manager->ConnectionClosed();

  // Since the user is still signed in, the auth error should have remained.
  EXPECT_NE(auth_manager->GetLastAuthError().state(),
            GoogleServiceAuthError::NONE);
}

TEST_F(SyncAuthManagerTest, BraveForwardsCredentialsEvents) {
  // Start out already signed in before the SyncAuthManager is created.
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;

  base::MockCallback<AccountStateChangedCallback> account_state_changed;
  base::MockCallback<CredentialsChangedCallback> credentials_changed;
  EXPECT_CALL(account_state_changed, Run()).Times(0);
  EXPECT_CALL(credentials_changed, Run()).Times(0);
  auto auth_manager =
      CreateAuthManager(account_state_changed.Get(), credentials_changed.Get());
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);

  auth_manager->RegisterForAuthNotifications();

  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();

  // Once an access token is available, the callback should get run.
  EXPECT_CALL(credentials_changed, Run());
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");

  // Now the refresh token gets updated. The access token will get dropped, so
  // this should cause another notification.
  EXPECT_CALL(credentials_changed, Run());
  identity_env()->SetRefreshTokenForPrimaryAccount();
  access_token_fetcher->SetAccessTokenResponseForTest(
      brave_sync::AccessTokenConsumer::TokenResponse(
          "access_token_2", base::Time::Now() + base::TimeDelta::FromHours(1)));
  CoreAccountInfo account_info;
  account_info.account_id = account_id;
  auth_manager->OnRefreshTokenUpdatedForAccount(account_info);
  ASSERT_TRUE(auth_manager->GetCredentials().access_token.empty());

  // Once a new token is available, there's another notification.
  EXPECT_CALL(credentials_changed, Run());
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token_2", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token_2");

  // Revoking the refresh token should also cause the access token to get
  // dropped.
  EXPECT_CALL(credentials_changed, Run());
  identity_env()->RemoveRefreshTokenForPrimaryAccount();
  auth_manager->OnRefreshTokenRemovedForAccount(account_id);
  EXPECT_TRUE(auth_manager->GetCredentials().access_token.empty());
}

TEST_F(SyncAuthManagerTest, BraveRequestsAccessTokenOnSyncStartup) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();

  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);

  EXPECT_EQ(auth_manager->GetCredentials().access_token, "access_token");
}

TEST_F(SyncAuthManagerTest,
       BraveRetriesAccessTokenFetchWithBackoffOnTransientFailure) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  access_token_fetcher->SetTokenResponseError(
      GoogleServiceAuthError::FromConnectionError(net::ERR_TIMED_OUT));
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();

  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithError(
  //     GoogleServiceAuthError::FromConnectionError(net::ERR_TIMED_OUT));
  WaitForAccessTokenResponse(access_token_fetcher);

  // The access token fetch should get retried (with backoff, hence no actual
  // request yet), without exposing an auth error.
  EXPECT_TRUE(auth_manager->IsRetryingAccessTokenFetchForTest());
  EXPECT_EQ(auth_manager->GetLastAuthError(),
            GoogleServiceAuthError::AuthErrorNone());
}

TEST_F(
SyncAuthManagerTest,
BraveRetriesAccessTokenFetchWithBackoffOnFirstCancelTransientFailWhenDisabled) {
  // Disable the first retry without backoff on cancellation.
  base::test::ScopedFeatureList local_feature;
  local_feature.InitAndDisableFeature(kSyncRetryFirstCanceledTokenFetch);

  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  access_token_fetcher->SetTokenResponseError(
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();

  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithError(
  //     GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));
  WaitForAccessTokenResponse(access_token_fetcher);

  // Expect retry with backoff.
  EXPECT_TRUE(auth_manager->IsRetryingAccessTokenFetchForTest());
}

TEST_F(SyncAuthManagerTest,
BraveRetriesAccessTokenFetchWithoutBackoffOnceOnFirstCancelTransientFailure) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  access_token_fetcher->SetTokenResponseError(
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();

  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithError(
  //     GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));
  access_token_fetcher->KeepTokenResponseErrorOnce();
  WaitForAccessTokenResponse(access_token_fetcher);

  // Expect no backoff the first time the request is canceled.
  EXPECT_FALSE(auth_manager->IsRetryingAccessTokenFetchForTest());

  // Cancel the retry as well.
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithError(
  //     GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));
  WaitForAccessTokenResponse(access_token_fetcher);

  // Expect retry with backoff when the first retry was also canceled.
  EXPECT_TRUE(auth_manager->IsRetryingAccessTokenFetchForTest());
}

TEST_F(SyncAuthManagerTest,
       BraveRetriesAccessTokenFetchOnFirstCancelTransientFailure) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  access_token_fetcher->SetTokenResponseError(
      GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();

  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithError(
  //     GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED));
  WaitForAccessTokenResponse(access_token_fetcher);

  // Expect no backoff the first time the request is canceled.
  EXPECT_FALSE(auth_manager->IsRetryingAccessTokenFetchForTest());

  // Retry is a success.
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);

  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");
  // Don't expect any backoff when the retry is a success.
  EXPECT_FALSE(auth_manager->IsRetryingAccessTokenFetchForTest());
}

TEST_F(SyncAuthManagerTest, BraveAbortsAccessTokenFetchOnPersistentFailure) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  GoogleServiceAuthError auth_error_1 =
      GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
          GoogleServiceAuthError::InvalidGaiaCredentialsReason::
              CREDENTIALS_REJECTED_BY_SERVER);
  access_token_fetcher->SetTokenResponseError(auth_error_1);
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();

  GoogleServiceAuthError auth_error =
      GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
          GoogleServiceAuthError::InvalidGaiaCredentialsReason::
              CREDENTIALS_REJECTED_BY_SERVER);
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithError(
  //     auth_error);
  WaitForAccessTokenResponse(access_token_fetcher);

  // Auth error should get exposed; no retry.
  EXPECT_FALSE(auth_manager->IsRetryingAccessTokenFetchForTest());
  EXPECT_EQ(auth_manager->GetLastAuthError(), auth_error);
}

TEST_F(SyncAuthManagerTest,
       BraveFetchesNewAccessTokenWithBackoffOnServerError) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");

  // The server is returning AUTH_ERROR - maybe something's wrong with the
  // token we got.
  auth_manager->ConnectionStatusChanged(syncer::CONNECTION_AUTH_ERROR);

  // The access token fetch should get retried (with backoff, hence no actual
  // request yet), without exposing an auth error.
  EXPECT_TRUE(auth_manager->IsRetryingAccessTokenFetchForTest());
  EXPECT_EQ(auth_manager->GetLastAuthError(),
            GoogleServiceAuthError::AuthErrorNone());
}

TEST_F(SyncAuthManagerTest, BraveExposesServerError) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");

  // Now a server error happens.
  auth_manager->ConnectionStatusChanged(syncer::CONNECTION_SERVER_ERROR);

  // The error should be reported.
  EXPECT_NE(auth_manager->GetLastAuthError(),
            GoogleServiceAuthError::AuthErrorNone());
  // But the access token should still be there - this might just be some
  // non-auth-related problem with the server.
  EXPECT_EQ(auth_manager->GetCredentials().access_token, "access_token");
}

TEST_F(SyncAuthManagerTest, BraveClearsServerErrorOnSyncDisable) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");

  // A server error happens.
  auth_manager->ConnectionStatusChanged(syncer::CONNECTION_SERVER_ERROR);
  ASSERT_NE(auth_manager->GetLastAuthError(),
            GoogleServiceAuthError::AuthErrorNone());

  // Now Sync gets turned off, e.g. because the user disabled it.
  auth_manager->ConnectionClosed();

  // This should have cleared the auth error, because it was due to a server
  // error which is now not meaningful anymore.
  EXPECT_EQ(auth_manager->GetLastAuthError(),
            GoogleServiceAuthError::AuthErrorNone());
}

TEST_F(SyncAuthManagerTest, BraveRequestsNewAccessTokenOnExpiry) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");

  // Now everything is okay for a while.
  auth_manager->ConnectionStatusChanged(syncer::CONNECTION_OK);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");
  ASSERT_EQ(auth_manager->GetLastAuthError(),
            GoogleServiceAuthError::AuthErrorNone());

  // But then the token expires, resulting in an auth error from the server.
  auth_manager->ConnectionStatusChanged(syncer::CONNECTION_AUTH_ERROR);

  // Should immediately drop the access token and fetch a new one (no backoff).
  EXPECT_TRUE(auth_manager->GetCredentials().access_token.empty());

  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token_2", base::Time::Now() + base::TimeDelta::FromHours(1));
  access_token_fetcher->SetAccessTokenResponseForTest(
      brave_sync::AccessTokenConsumer::TokenResponse(
          "access_token_2",
          base::Time::Now() + base::TimeDelta::FromHours(1)));
  WaitForAccessTokenResponse(access_token_fetcher);
  EXPECT_EQ(auth_manager->GetCredentials().access_token, "access_token_2");
}

TEST_F(SyncAuthManagerTest, BraveRequestsNewAccessTokenOnRefreshTokenUpdate) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");

  // Now everything is okay for a while.
  auth_manager->ConnectionStatusChanged(syncer::CONNECTION_OK);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");
  ASSERT_EQ(auth_manager->GetLastAuthError(),
            GoogleServiceAuthError::AuthErrorNone());

  // But then the refresh token changes.
  identity_env()->SetRefreshTokenForPrimaryAccount();
  access_token_fetcher->SetAccessTokenResponseForTest(
      brave_sync::AccessTokenConsumer::TokenResponse(
          "access_token_2",
          base::Time::Now() + base::TimeDelta::FromHours(1)));
  CoreAccountInfo account_info;
  account_info.account_id = account_id;
  auth_manager->OnRefreshTokenUpdatedForAccount(account_info);

  // Should immediately drop the access token and fetch a new one (no backoff).
  EXPECT_TRUE(auth_manager->GetCredentials().access_token.empty());

  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token_2", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);
  EXPECT_EQ(auth_manager->GetCredentials().access_token, "access_token_2");
}

TEST_F(SyncAuthManagerTest, BraveDoesNotRequestAccessTokenAutonomously) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  // Do *not* call ConnectionStatusChanged here (which is what usually kicks off
  // the token fetch).

  // Now the refresh token gets updated. If we already had an access token
  // before, then this should trigger a new fetch. But since that initial fetch
  // never happened (e.g. because Sync is turned off), this should do nothing.
  base::MockCallback<base::OnceClosure> access_token_requested;
  EXPECT_CALL(access_token_requested, Run()).Times(0);
  identity_env()->SetCallbackForNextAccessTokenRequest(
      access_token_requested.Get());
  identity_env()->SetRefreshTokenForPrimaryAccount();
  access_token_fetcher->SetTokenResponseCallback(access_token_requested.Get());
  CoreAccountInfo account_info;
  account_info.account_id = account_id;
  auth_manager->OnRefreshTokenUpdatedForAccount(account_info);

  // Make sure no access token request was sent. Since the request goes through
  // posted tasks, we have to spin the message loop.
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(auth_manager->GetCredentials().access_token.empty());
}

TEST_F(SyncAuthManagerTest, BraveClearsCredentialsOnRefreshTokenRemoval) {
  CoreAccountId account_id =
      identity_env()->MakePrimaryAccountAvailable("test@email.com").account_id;
  auto auth_manager = CreateAuthManager();
  account_id = CoreAccountId::FromString(account_id_str);
  auto* access_token_fetcher = CreateAccessTokenFetcher(auth_manager.get());
  auth_manager->DeriveSigningKeys(sync_code1);
  auth_manager->RegisterForAuthNotifications();
  ASSERT_EQ(auth_manager->GetActiveAccountInfo().account_info.account_id,
            account_id);

  auth_manager->ConnectionOpened();
  // identity_env()->WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
  //     "access_token", base::Time::Now() + base::TimeDelta::FromHours(1));
  WaitForAccessTokenResponse(access_token_fetcher);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");

  // Now everything is okay for a while.
  auth_manager->ConnectionStatusChanged(syncer::CONNECTION_OK);
  ASSERT_EQ(auth_manager->GetCredentials().access_token, "access_token");
  ASSERT_EQ(auth_manager->GetLastAuthError(),
            GoogleServiceAuthError::AuthErrorNone());

  // But then the refresh token gets revoked. No new access token should get
  // requested due to this.
  base::MockCallback<base::OnceClosure> access_token_requested;
  EXPECT_CALL(access_token_requested, Run()).Times(0);
  identity_env()->SetCallbackForNextAccessTokenRequest(
      access_token_requested.Get());
  identity_env()->RemoveRefreshTokenForPrimaryAccount();
  access_token_fetcher->SetTokenResponseCallback(access_token_requested.Get());
  auth_manager->OnRefreshTokenRemovedForAccount(account_id);

  // Should immediately drop the access token and expose an auth error.
  EXPECT_TRUE(auth_manager->GetCredentials().access_token.empty());
  EXPECT_NE(auth_manager->GetLastAuthError(),
            GoogleServiceAuthError::AuthErrorNone());

  // No new access token should have been requested. Since the request goes
  // through posted tasks, we have to spin the message loop.
  base::RunLoop().RunUntilIdle();
}

}  // namespace syncer
