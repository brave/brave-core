/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/test/base/browser_with_test_window_test.h"

#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"

// Verify BrowserWithTestWindowTest works with brave.
TEST_F(BrowserWithTestWindowTest, BasicTest) {
  AddTab(browser(), GURL(chrome::kChromeUINewTabURL));
}
