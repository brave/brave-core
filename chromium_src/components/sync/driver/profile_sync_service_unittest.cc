/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "components/os_crypt/os_crypt_mocker.h"

namespace {
const char sync_code[] =
    "badge unique kiwi orient spring venue piano "
    "lake admit ill roof brother grant hour better "
    "proud cabbage fee slow economy wage final fox cancel";
const char account_id[] = "gaia_id_for_user_gmail.com";
}  // namespace

#define EnableSyncSignOutAndChangeAccount \
  DISABLED_EnableSyncSignOutAndChangeAccount
#define RevokeAccessTokenFromTokenService \
  DISABLED_RevokeAccessTokenFromTokenService
#define CredentialsRejectedByClient_StopSync \
  DISABLED_CredentialsRejectedByClient_StopSync
#define CredentialsRejectedByClient_DoNotStopSync \
  DISABLED_CredentialsRejectedByClient_DoNotStopSync
#define CredentialErrorReturned DISABLED_CredentialErrorReturned
#define CredentialErrorClearsOnNewToken DISABLED_CredentialErrorClearsOnNewToken
#define DisableSyncOnClient DISABLED_DisableSyncOnClient
#define GetUserNoisedBirthYearAndGender_SyncPausedAndFeatureDisabled \
  DISABLED_GetUserNoisedBirthYearAndGender_SyncPausedAndFeatureDisabled
#define GetUserNoisedBirthYearAndGender_SyncPausedAndFeatureEnabled \
  DISABLED_GetUserNoisedBirthYearAndGender_SyncPausedAndFeatureEnabled
#define GetExperimentalAuthenticationKey \
  DISABLED_GetExperimentalAuthenticationKey
#define ShouldProvideDisableReasonsAfterShutdown \
  DISABLED_ShouldProvideDisableReasonsAfterShutdown

#define BRAVE_SET_UP                           \
  OSCryptMocker::SetUp();                      \
  brave_sync::Prefs brave_sync_prefs(prefs()); \
  brave_sync_prefs.SetSyncV1Migrated(true);
#define BRAVE_TEAR_DOWN OSCryptMocker::TearDown();
#define BRAVE_SIGN_IN                          \
  brave_sync::Prefs brave_sync_prefs(prefs()); \
  brave_sync_prefs.SetSeed(sync_code);
#define BRAVE_GET_PRIMARY_ACCOUNT CoreAccountId::FromString(account_id);
#define BRAVE_SIGN_OUT                         \
  brave_sync::Prefs brave_sync_prefs(prefs()); \
  brave_sync_prefs.Clear();

#include "../../../../../components/sync/driver/profile_sync_service_unittest.cc"
#undef EnableSyncSignOutAndChangeAccount
#undef RevokeAccessTokenFromTokenService
#undef CredentialsRejectedByClient_StopSync
#undef CredentialsRejectedByClient_DoNotStopSync
#undef CredentialErrorReturned
#undef CredentialErrorClearsOnNewToken
#undef DisableSyncOnClient
#undef GetUserNoisedBirthYearAndGender_SyncPausedAndFeatureDisabled
#undef GetUserNoisedBirthYearAndGender_SyncPausedAndFeatureEnabled
#undef GetExperimentalAuthenticationKey
#undef ShouldProvideDisableReasonsAfterShutdown

#undef BRAVE_SET_UP
#undef BRAVE_TEAR_DOWN
#undef BRAVE_SIGN_IN
#undef BRAVE_GET_PRIMARY_ACCOUNT
#undef BRAVE_SIGN_OUT
