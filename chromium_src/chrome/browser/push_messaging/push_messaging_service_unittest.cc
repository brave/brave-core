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

#define TestingProfile BraveTestingProfile
#include "src/chrome/browser/push_messaging/push_messaging_service_unittest.cc"
#undef TestingProfile
#undef RecordsRevocationAndSourceUiWithReporterTest
#undef RecordsRevocationAndSourceUiNoReporterTest
