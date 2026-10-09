// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/web_ui_mocha_browser_test.h"
#include "content/public/test/browser_test.h"

// The suites live in chromium_src/chrome/test/data/webui/settings/. They check
// that Brave's overrides of the Lit settings elements still apply, as they do
// so silently when upstream changes an element.
class BraveSettingsOverridesWebUITest : public WebUIMochaBrowserTest {
 protected:
  BraveSettingsOverridesWebUITest() {
    set_test_loader_host(chrome::kChromeUISettingsHost);
  }
};

IN_PROC_BROWSER_TEST_F(BraveSettingsOverridesWebUITest, Menu) {
  RunTest("settings/settings_menu_test.js",
          "runMochaSuite('BraveSettingsMenu')");
}

IN_PROC_BROWSER_TEST_F(BraveSettingsOverridesWebUITest, UI) {
  RunTest("settings/settings_ui_test.js", "runMochaSuite('BraveSettingsUI')");
}

IN_PROC_BROWSER_TEST_F(BraveSettingsOverridesWebUITest, AutofillPage) {
  RunTest("settings/autofill_page_test.js",
          "runMochaSuite('BraveAutofillPage')");
}

IN_PROC_BROWSER_TEST_F(BraveSettingsOverridesWebUITest, ResetProfileDialog) {
  RunTest("settings/reset_page_test.js",
          "runMochaSuite('BraveResetProfileDialog')");
}

IN_PROC_BROWSER_TEST_F(BraveSettingsOverridesWebUITest, SyncControls) {
  RunTest("settings/people_page_sync_controls_test.js",
          "runMochaSuite('BraveSyncControls')");
}

IN_PROC_BROWSER_TEST_F(BraveSettingsOverridesWebUITest, PrivacyPage) {
  RunTest("settings/privacy_page_test.js", "runMochaSuite('BravePrivacyPage')");
}

IN_PROC_BROWSER_TEST_F(BraveSettingsOverridesWebUITest, MemoryPage) {
  RunTest("settings/memory_page_test.js", "runMochaSuite('BraveMemoryPage')");
}

IN_PROC_BROWSER_TEST_F(BraveSettingsOverridesWebUITest, OriginPage) {
  RunTest("settings/settings_main_test.js", "runMochaSuite('BraveOriginPage')");
}
