/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/common/extension_urls.h"

#include "base/command_line.h"
#include "brave/components/update_client/buildflags.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/component_updater/component_updater_switches.h"
#include "content/public/test/browser_test.h"

using ExtensionUrlsBrowserTest = PlatformBrowserTest;

IN_PROC_BROWSER_TEST_F(ExtensionUrlsBrowserTest, IsWebstoreUpdateUrl) {
  GURL url = GURL(extension_urls::kChromeWebstoreUpdateURL);
  EXPECT_TRUE(extension_urls::IsWebstoreUpdateUrl(url));

  url = GURL(BUILDFLAG(UPDATER_PROD_ENDPOINT));
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kComponentUpdater));
  EXPECT_TRUE(extension_urls::IsWebstoreUpdateUrl(url));
}
