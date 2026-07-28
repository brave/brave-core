/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/common/extension_urls.h"

#include <string_view>

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

  // `BraveMainDelegate` only points the component updater at Brave's endpoint
  // when one is baked into the build. `updater_prod_endpoint` is only required
  // to be non-empty for official builds (see
  // //brave/components/update_client/BUILD.gn), and builds without it keep the
  // upstream update URL, so there is nothing Brave-specific left to verify.
  // `if constexpr` is required here: with an empty endpoint a plain `if` leaves
  // the code below unreachable, which `-Wunreachable-code` rejects.
  constexpr std::string_view kUpdaterProdEndpoint =
      BUILDFLAG(UPDATER_PROD_ENDPOINT);
  if constexpr (kUpdaterProdEndpoint.empty()) {
    GTEST_SKIP() << "updater_prod_endpoint is empty in this build";
  }

  url = GURL(kUpdaterProdEndpoint);
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kComponentUpdater));
  EXPECT_TRUE(extension_urls::IsWebstoreUpdateUrl(url));
}
