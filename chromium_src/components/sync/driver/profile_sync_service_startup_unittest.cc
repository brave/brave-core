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
}  // namespace

#define BRAVE_PROFILE_SYNC_SERVICE_STARTUP_TEST_      \
  void SetUp() override {                             \
    testing::Test::SetUp();                           \
    OSCryptMocker::SetUp();                           \
    brave_sync::Prefs brave_sync_prefs(               \
        profile_sync_service_bundle_.pref_service()); \
    brave_sync_prefs.SetSyncV1Migrated(true);         \
  }                                                   \
  void TearDown() override {                          \
    OSCryptMocker::TearDown();                        \
    testing::Test::TearDown();                        \
  }

#define BRAVE_SIMULATE_TEST_USER_SIGNIN             \
  brave_sync::Prefs brave_sync_prefs(               \
      profile_sync_service_bundle_.pref_service()); \
  brave_sync_prefs.SetSeed(sync_code);

#define BRAVE_SIMULATE_TEST_USER_SIGNIN_WITHOUT_REFRESH_TOKEN \
  brave_sync::Prefs brave_sync_prefs(                         \
      profile_sync_service_bundle_.pref_service());           \
  brave_sync_prefs.SetSeed(sync_code);

#include "../../../../../components/sync/driver/profile_sync_service_startup_unittest.cc"
#undef BRAVE_PROFILE_SYNC_SERVICE_STARTUP_TEST_
#undef BRAVE_SIMULATE_TEST_USER_SIGNIN
#undef BRAVE_SIMULATE_TEST_USER_SIGNIN_WITHOUT_REFRESH_TOKEN
