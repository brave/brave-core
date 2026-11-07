// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/web_ui_mocha_browser_test.h"
#include "content/public/test/browser_test.h"

class BraveSettingsToggleWebUITest : public WebUIMochaBrowserTest {
 protected:
  BraveSettingsToggleWebUITest() {
    set_test_loader_host(chrome::kChromeUISettingsHost);
  }
};

// Brave pages bind their toggles by `pref-key`, since upstream removed the
// top-level `prefs` object. The suite lives in
// chromium_src/chrome/test/data/webui/settings/settings_toggle_button_test.ts.
IN_PROC_BROWSER_TEST_F(BraveSettingsToggleWebUITest, PrefKeyToggles) {
  RunTest("settings/settings_toggle_button_test.js",
          "runMochaSuite('BraveSettingsToggleButtonPrefKey')");
}
