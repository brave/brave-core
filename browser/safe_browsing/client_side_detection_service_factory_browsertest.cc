// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/safe_browsing/client_side_detection/client_side_detection_service_factory.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/test/browser_test.h"

using BraveClientSideDetectionServiceFactoryBrowserTest = PlatformBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveClientSideDetectionServiceFactoryBrowserTest,
                       NoFactoryOrService) {
  EXPECT_EQ(safe_browsing::ClientSideDetectionServiceFactory::GetInstance(),
            nullptr);
  EXPECT_EQ(safe_browsing::ClientSideDetectionServiceFactory::GetForProfile(
                chrome_test_utils::GetProfile(this)),
            nullptr);
}
