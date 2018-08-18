/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_webstore_inline_installer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

void OnInstalled(bool success, const std::string& error,
    extensions::webstore_install::Result result) {
}

class BraveWebstoreBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    extension_id_ = "apdfllckaahabafndbhieahigkjlhalf";
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    ui_test_utils::NavigateToURL(browser(), url);
    return WaitForLoadStop(contents());
  }

  int GetTabCount() const {
    return  browser()->tab_strip_model()->count();
  }

  bool CheckInlineInstallPermitted() {
    base::DictionaryValue webstore_data;
    std::string err;
    auto* installer = new extensions::BraveWebstoreInlineInstaller(contents(),
        contents()->GetMainFrame(), extension_id(), GURL(),
        base::Bind(&OnInstalled));
    return installer->CheckInlineInstallPermitted(webstore_data, &err);
  }

  std::string extension_id() const {
    return extension_id_;
  }

 protected:
  std::string extension_id_;
};

IN_PROC_BROWSER_TEST_F(BraveWebstoreBrowserTest,
    RedirectsUserToChromeWebStore) {
  // Inline install should not be permitted.
  ASSERT_FALSE(CheckInlineInstallPermitted());
  // Inline install should create a second tab for the web-contents.
  GURL url(base::StringPrintf(extensions::kWebstoreUrlFormat,
      extension_id().c_str()));
  ASSERT_EQ(GetTabCount(), 2);
  ASSERT_STREQ(browser()->tab_strip_model()->GetWebContentsAt(1)->GetVisibleURL().spec().c_str(), url.spec().c_str());
}
