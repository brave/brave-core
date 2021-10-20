/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/extensions/brave_extension_functional_test.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

class BraveExtensionProviderTest : public extensions::ExtensionFunctionalTest {
 public:
  void SetUpOnMainThread() override {
    extensions::ExtensionFunctionalTest::SetUpOnMainThread();
  }
};

namespace extensions {

IN_PROC_BROWSER_TEST_F(BraveExtensionProviderTest, BlacklistExtension) {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  const extensions::Extension* extension = InstallExtension(
      test_data_dir.AppendASCII("should-be-blocked-extension"), 0);
  ASSERT_FALSE(extension);
}

IN_PROC_BROWSER_TEST_F(BraveExtensionProviderTest, WhitelistedExtension) {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  const extensions::Extension* extension = InstallExtension(
      test_data_dir.AppendASCII("adblock-data").AppendASCII("adblock-default"),
      1);
  ASSERT_TRUE(extension);
}

// Load an extension page with an ad image, and make sure it is NOT blocked.
// It would otherwise be blocked though if it wasn't an extension.
IN_PROC_BROWSER_TEST_F(BraveExtensionProviderTest,
                       AdsNotBlockedByDefaultBlockerInExtension) {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  scoped_refptr<const extensions::Extension> extension =
      InstallExtensionSilently(
          extension_service(),
          test_data_dir.AppendASCII("extension-compat-test-extension.crx"));
  GURL url = GURL(std::string(kChromeExtensionScheme) + "://" +
                  extension->id() + "/blocking.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "setExpectations(1, 0, 0, 0);"
      "addImage('ad_banner.png')",
      &as_expected));
  EXPECT_TRUE(as_expected);
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

IN_PROC_BROWSER_TEST_F(BraveExtensionProviderTest, ExtensionsCanGetCookies) {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  scoped_refptr<const extensions::Extension> extension =
      InstallExtensionSilently(
      extension_service(),
      test_data_dir.AppendASCII("extension-compat-test-extension.crx"));
  GURL url = GURL(std::string(kChromeExtensionScheme) + "://" +
                  extension->id() + "/blocking.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "canGetCookie('test', 'http://a.com')",
      &as_expected));
  EXPECT_TRUE(as_expected);
}

IN_PROC_BROWSER_TEST_F(BraveExtensionProviderTest, ExtensionsCanSetCookies) {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  scoped_refptr<const extensions::Extension> extension =
      InstallExtensionSilently(
      extension_service(),
      test_data_dir.AppendASCII("extension-compat-test-extension.crx"));
  GURL url = GURL(std::string(kChromeExtensionScheme) + "://" +
                  extension->id() + "/blocking.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  bool as_expected = false;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "canSetCookie('test', 'testval', 'http://a.com')",
      &as_expected));
  EXPECT_TRUE(as_expected);
}

}  // namespace extensions
