/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"


#define BRAVE_PROFILE_SYNC_SERVICE_STARTUP_TEST_      \
  void SetUp() override {                             \
    brave_sync::Prefs brave_sync_prefs(               \
        profile_sync_service_bundle_.pref_service()); \
    brave_sync_prefs.SetSyncV1Migrated(true);         \
  }

#include "../../../../../components/sync/driver/profile_sync_service_startup_unittest.cc"

#undef BRAVE_PROFILE_SYNC_SERVICE_STARTUP_TEST_
