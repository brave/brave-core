// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

namespace brave_wallet {

class TrezorUIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    ASSERT_TRUE(embedded_test_server()->Start());
  }
};

IN_PROC_BROWSER_TEST_F(TrezorUIBrowserTest, CheckOpenerInPopup) {
  auto* trezor_bridge = ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(kUntrustedTrezorURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  content::WebContentsAddedObserver window_observer;

  // Trezor connect tries to open popup this way. Make sure our patch works and
  // do this in a bit different way so opened window has `opener` set.
  EXPECT_TRUE(content::ExecJs(
      trezor_bridge,
      content::JsReplace(
          "window.open($1, 'modal')",
          embedded_test_server()->GetURL("/empty.html").spec())));

  content::WebContents* popup = window_observer.GetWebContents();
  EXPECT_TRUE(WaitForLoadStop(popup));

  // Ensure there is non-null `opener` in opened window.
  EXPECT_TRUE(content::EvalJs(popup, "!!window.opener").ExtractBool());
}

}  // namespace brave_wallet
