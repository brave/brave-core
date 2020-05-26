/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// brave sync doesn't have pause state
#define SyncPausedState DISABLED_SyncPausedState
#define ShouldTrackDeletionsInSyncPausedState \
  DISABLED_ShouldTrackDeletionsInSyncPausedState
#define ShouldRecordNigoriConfigurationWithInvalidatedCredentials \
  DISABLED_ShouldRecordNigoriConfigurationWithInvalidatedCredentials
#include "../../../../../../../chrome/browser/sync/test/integration/sync_auth_test.cc"
#undef SyncPausedState
#undef ShouldTrackDeletionsInSyncPausedState
#undef ShouldRecordNigoriConfigurationWithInvalidatedCredentials
