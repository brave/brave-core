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

#define TestingProfile BraveTestingProfile
#include <chrome/browser/push_messaging/push_messaging_service_unittest.cc>
#undef TestingProfile
#undef ProfileDestructionTest
#undef RecordsRevocationAndSourceUiWithReporterTest
#undef RecordsRevocationAndSourceUiNoReporterTest
