/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/infobar_utils.h"

#include "base/command_line.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/startup/startup_types.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_delegate.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_test.h"
#include "google_apis/google_api_keys.h"

using InfoBarUtilsBrowserTest = InProcessBrowserTest;

// We don't set a Google API key, and upstream's "Missing Google API keys"
// infobar must stay hidden.
IN_PROC_BROWSER_TEST_F(InfoBarUtilsBrowserTest, NoGoogleApiKeysInfoBar) {
  ASSERT_FALSE(google_apis::HasAPIKeyConfigured());

  // An empty command line, so that the startup code doesn't skip the infobars
  // for the test harness' `--test-type`.
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  BrowserWindowInterface* window = browser();
  AddInfoBarsIfNecessary(window, window->GetProfile(), command_line,
                         chrome::startup::IsFirstRun::kNo,
                         /*is_web_app=*/false, /*is_post_crash_launch=*/false,
                         /*was_restarted=*/false);

  auto* manager = infobars::ContentInfoBarManager::FromWebContents(
      window->GetActiveTabInterface()->GetContents());
  ASSERT_TRUE(manager);
  for (infobars::InfoBar* infobar : manager->infobars()) {
    EXPECT_NE(infobar->delegate()->GetIdentifier(),
              infobars::InfoBarDelegate::GOOGLE_API_KEYS_INFOBAR_DELEGATE);
  }
}
