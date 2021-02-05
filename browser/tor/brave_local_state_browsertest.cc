/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_factory.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"


using BraveLocalStateBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveLocalStateBrowserTest, BasicTest) {
    // Tor is enabled by default.
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled());
}
