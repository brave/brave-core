/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/devtools/devtools_browsertest.cc"

#include "extensions/common/switches.h"

class BraveDevToolsExtensionTest : public DevToolsExtensionTest,
                                   public ::testing::WithParamInterface<bool> {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    if (GetParam()) {
      command_line->AppendSwitch(extensions::switches::kExtensionsOnChromeURLs);
    }
  }

  // Only available for non-official builds when command line flag is set.
  bool AllowExtensionsForDevtoolsOnChromeScheme() {
#if defined(OFFICIAL_BUILD)
    return false;
#else
    return GetParam();
#endif
  }
};

IN_PROC_BROWSER_TEST_P(BraveDevToolsExtensionTest, InspectChromeScheme) {
  if (AllowExtensionsForDevtoolsOnChromeScheme()) {
    // Tests that certain devtools APIs are exposed to extension on chrome
    // scheme page.
    LoadExtension("devtools_extension");
    RunTest("waitForTestResultsInConsole", "chrome://version/");
  } else {
    // Redirects to chrome://version and expects dev tools to disable extension.
    LoadExtension("can_inspect_url");
    RunTest("waitForTestResultsAsMessage",
            base::StrCat({kArbitraryPage, "#chrome://version/"}));
  }
}

IN_PROC_BROWSER_TEST_P(BraveDevToolsExtensionTest, InspectHttpsScheme) {
  // Tests that certain devtools APIs are exposed to extension on https scheme
  // page.
  LoadExtension("devtools_extension");
  RunTest("waitForTestResultsInConsole", kArbitraryPage);
}

INSTANTIATE_TEST_SUITE_P(All,
                         BraveDevToolsExtensionTest,
                         ::testing::Bool(),
                         [](const testing::TestParamInfo<
                             BraveDevToolsExtensionTest::ParamType>& info) {
                           return info.param ? "ExtensionsOnChromeURLEnabled"
                                             : "ExtensionsOnChromeURLDisabled";
                         });
