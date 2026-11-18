// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/web_ui_mocha_browser_test.h"
#include "content/public/test/browser_test.h"

// The suites live in chromium_src/chrome/test/data/webui/history/. They check
// that Brave's overrides of the history page still apply, as they do so
// silently when upstream changes it.
class BraveHistoryOverridesWebUITest : public WebUIMochaBrowserTest {
 protected:
  BraveHistoryOverridesWebUITest() {
    set_test_loader_host(chrome::kChromeUIHistoryHost);
  }
};

IN_PROC_BROWSER_TEST_F(BraveHistoryOverridesWebUITest, SyncedDevices) {
  RunTest("history/history_synced_tabs_test.js",
          "runMochaSuite('BraveSyncedDevices')");
}
