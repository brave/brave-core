/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "third_party/blink/public/common/features.h"
#include "url/gurl.h"

namespace {

constexpr char kTestPage[] = "/simple.html";

// Reports what a page can see of the Window Management API. `SecureContext`
// hides these members on its own, so `isSecureContext` is reported to keep the
// disabled case from passing for the wrong reason.
constexpr char kProbeScript[] = R"(
  (async () => {
    let permissionError = null;
    try {
      await navigator.permissions.query({name: 'window-management'});
    } catch (e) {
      permissionError = e.name;
    }
    return JSON.stringify({
      isSecureContext: window.isSecureContext,
      getScreenDetails: typeof window.getScreenDetails,
      ScreenDetails: typeof window.ScreenDetails,
      ScreenDetailed: typeof window.ScreenDetailed,
      isExtended: 'isExtended' in window.screen,
      onchange: 'onchange' in window.screen,
      permissionError,
    });
  })()
)";

constexpr char kExpectedDisabled[] =
    R"({"isSecureContext":true,"getScreenDetails":"undefined",)"
    R"("ScreenDetails":"undefined","ScreenDetailed":"undefined",)"
    R"("isExtended":false,"onchange":false,"permissionError":"TypeError"})";

constexpr char kExpectedEnabled[] =
    R"({"isSecureContext":true,"getScreenDetails":"function",)"
    R"("ScreenDetails":"function","ScreenDetailed":"function",)"
    R"("isExtended":true,"onchange":true,"permissionError":null})";

}  // namespace

class WindowManagementBrowserTestBase : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    embedded_test_server()->ServeFilesFromDirectory(
        base::PathService::CheckedGet(brave::DIR_TEST_DATA));

    ASSERT_TRUE(embedded_test_server()->Start());
  }

 protected:
  content::EvalJsResult Probe() {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL(kTestPage)));
    return content::EvalJs(browser()->tab_strip_model()->GetActiveWebContents(),
                           kProbeScript);
  }
};

class WindowManagementDisabledTest : public WindowManagementBrowserTestBase {};

IN_PROC_BROWSER_TEST_F(WindowManagementDisabledTest, IsDisabled) {
  EXPECT_EQ(kExpectedDisabled, Probe());
}

// The `brave://flags` entry toggles this base::Feature, which is copied into
// the runtime flag when the renderer starts.
class WindowManagementFeatureEnabledTest
    : public WindowManagementBrowserTestBase {
 public:
  WindowManagementFeatureEnabledTest() {
    feature_list_.InitAndEnableFeature(blink::features::kBraveWindowManagement);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(WindowManagementFeatureEnabledTest,
                       IsExposedWhenFeatureIsOn) {
  EXPECT_EQ(kExpectedEnabled, Probe());
}
