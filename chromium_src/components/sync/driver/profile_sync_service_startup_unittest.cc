#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "components/os_crypt/os_crypt_mocker.h"

#define BRAVE_PROFILE_SYNC_SERVICE_STARTUP_TEST_ \
  void SetUp() override {                        \
    testing::Test::SetUp();                      \
    OSCryptMocker::SetUp();                      \
  }                                              \
  void TearDown() override {                     \
    OSCryptMocker::TearDown();                   \
    testing::Test::TearDown();                   \
  }

#define BRAVE_SIMULATE_TEST_USER_SIGNIN                  \
  brave_sync::Prefs brave_sync_prefs(                    \
      profile_sync_service_bundle_.pref_service());      \
  brave_sync_prefs.SetSyncEnabled(true);                 \
  brave_sync_prefs.SetSeed(                              \
      "rabbit van once transfer town hammer humble "     \
      "unfold color gospel hunt ten urban injury sure "  \
      "tape stamp youth case armor caught normal share " \
      "fury");

#define BRAVE_SIMULATE_TEST_USER_SIGNIN_WITHOUT_REFRESH_TOKEN \
  brave_sync::Prefs brave_sync_prefs(                         \
      profile_sync_service_bundle_.pref_service());           \
  brave_sync_prefs.SetSyncEnabled(true);

#include "../../../../../components/sync/driver/profile_sync_service_startup_unittest.cc"
#undef BRAVE_PROFILE_SYNC_SERVICE_STARTUP_TEST_
#undef BRAVE_SIMULATE_TEST_USER_SIGNIN
#undef BRAVE_SIMULATE_TEST_USER_SIGNIN_WITHOUT_REFRESH_TOKEN
