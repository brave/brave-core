#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "components/os_crypt/os_crypt_mocker.h"

#define EnableSyncSignOutAndChangeAccount \
  DISABLED_EnableSyncSignOutAndChangeAccount
#define RevokeAccessTokenFromTokenService \
  DISABLED_RevokeAccessTokenFromTokenService
#define CredentialsRejectedByClient_StopSync \
  DISABLED_CredentialsRejectedByClient_StopSync
#define CredentialsRejectedByClient_DoNotStopSync \
  DISABLED_CredentialsRejectedByClient_DoNotStopSync
#define SignOutRevokeAccessToken DISABLED_SignOutRevokeAccessToken
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

#define BRAVE_SET_UP OSCryptMocker::SetUp();
#define BRAVE_TEAR_DOWN OSCryptMocker::TearDown();
#define BRAVE_SIGN_IN                                    \
  brave_sync::Prefs brave_sync_prefs(prefs());           \
  brave_sync_prefs.SetSyncEnabled(true);                 \
  brave_sync_prefs.SetSeed(                              \
      "rabbit van once transfer town hammer humble "     \
      "unfold color gospel hunt ten urban injury sure "  \
      "tape stamp youth case armor caught normal share " \
      "fury");

#include "../../../../../components/sync/driver/profile_sync_service_unittest.cc"
#undef EnableSyncSignOutAndChangeAccount
#undef RevokeAccessTokenFromTokenService
#undef CredentialsRejectedByClient_StopSync
#undef CredentialsRejectedByClient_DoNotStopSync
#undef SignOutRevokeAccessToken
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
