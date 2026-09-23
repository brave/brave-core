/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/test/base/brave_testing_profile.h"

// These tests fail because we do not record permissions UKM, see
// brave/chromium_src/components/permissions/permission_uma_util.cc
// GetUkmSourceId override.
#define RecordsRevocationAndSourceUiNoReporterTest \
  DISABLED_RecordsRevocationAndSourceUiNoReporterTest
#define RecordsRevocationAndSourceUiWithReporterTest \
  DISABLED_RecordsRevocationAndSourceUiWithReporterTest

// This test fails because we disable features::kDestroyProfileOnBrowserClose,
// which allows PushMessagingServiceImpl::OnMessage to find a profile to keep
// alive and then be able to dispatch the message. We disable the feature to
// make clear browsing data on exit to work.
#define ProfileDestructionTest DISABLED_ProfileDestructionTest

// These tests fail because a cr149 change in the tests swaps GCMProfileService
// to a fake one after creating a test profile.However our EphemeralService has
// a dependency chain on SyncInvalidationsService that creates an FCMHandler
// that stores a pointer to GCMDriver, so when the swap happens that pointer
// becomes invalid and when SyncInvalidationsService cleans it up in d'tor it
// causes a crash.
// https://source.chromium.org/chromium/chromium/src/+/23f0c1d6
#define PushMessagingAPIPermission DISABLED_PushMessagingAPIPermission
#define GetSubscriptionPersistsUserVisibleOnlyFalse \
  DISABLED_GetSubscriptionPersistsUserVisibleOnlyFalse

#define TestingProfile BraveTestingProfile
#include <chrome/browser/push_messaging/push_messaging_service_unittest.cc>
#undef TestingProfile
#undef GetSubscriptionPersistsUserVisibleOnlyFalse
#undef PushMessagingAPIPermission
#undef ProfileDestructionTest
#undef RecordsRevocationAndSourceUiWithReporterTest
#undef RecordsRevocationAndSourceUiNoReporterTest
