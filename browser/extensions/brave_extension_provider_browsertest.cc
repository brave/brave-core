/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/extensions/brave_extension_functional_test.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/url_constants.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

namespace extensions {

using BraveExtensionProviderTest = ExtensionFunctionalTest;

// Load an extension page with an ad image, and make sure it is NOT blocked.
// It would otherwise be blocked though if it wasn't an extension.
IN_PROC_BROWSER_TEST_F(BraveExtensionProviderTest,
                       AdsNotBlockedByDefaultBlockerInExtension) {
  scoped_refptr<const extensions::Extension> extension =
      InstallExtensionSilently("extension-compat-test-extension.crx",
                               "cdoagmgkjelodcdljmbjiifapnilecob");
  GURL url = extension->GetResourceURL("blocking.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetURL());

  EXPECT_EQ(true, content::EvalJs(contents,
                                  "setExpectations(1, 0, 0, 0);"
                                  "addImage('ad_banner.png')"));
  EXPECT_EQ(browser()->profile()->GetPrefs()->GetUint64(kAdsBlocked), 0ULL);
}

IN_PROC_BROWSER_TEST_F(BraveExtensionProviderTest, ExtensionsCanGetCookies) {
  base::FilePath src =
      GetTestDataDir().AppendASCII("extension-compat-test-extension");
  base::FilePath dest =
      GetTestDataDir().AppendASCII("extension-compat-test-extension-copy");
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::CopyDirectory(src, dest, false));
  }
  scoped_refptr<const extensions::Extension> extension =
      InstallUnpackedExtensionSilently(
          "extension-compat-test-extension-copy",
          "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAnsTdWGAO7gvCgM/"
          "ymAuEQ+OpT5T7zGj6UUR/ArzRvdM4RcU97O8Qnq86XSxwKdd/DjqsxGSimU5vw/"
          "WS4Xvos7ZnrSKy9oqo1ahPa7IQKnPNbs4OVwuI7HBnuskONveGcSH3LL+"
          "Vx5CDYpbjbgQMtOxEX3xO8u/"
          "MjAyzkt26XKS1jlsKbwY5yD38IsB9ldBVTU7oHMCA0pJpyQ0J4eKFtb0GdqUlUgpK/"
          "KYb+xP30Z81RzHXpdhXNN+"
          "jMQV8M9zox7FeWTGoKkE2faZcXn7VP88Gw0i8enZpR9JGD9fSexJ/"
          "IW9BzlkjEk8EI6pM309qGxe0ctj20a0MVcZDCLsGaQIDAQAB",
          "amcdfjbbjngdcepnmopaocdhglmfmihc");
  // InstallExtensionSilently("extension-compat-test-extension.crx",
  //                          "cdoagmgkjelodcdljmbjiifapnilecob");
  GURL url = extension->GetResourceURL("blocking.html");
  VLOG(1) << "BraveExtensionProviderTest: url = " << url.spec();

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetURL());
  VLOG(1) << "BraveExtensionProviderTest: Navigated to url = " << url.spec();

  EXPECT_EQ(true,
            content::EvalJs(contents, "canGetCookie('test', 'http://a.com')"));
}

IN_PROC_BROWSER_TEST_F(BraveExtensionProviderTest, ExtensionsCanSetCookies) {
  scoped_refptr<const extensions::Extension> extension =
      InstallExtensionSilently("extension-compat-test-extension.crx",
                               "cdoagmgkjelodcdljmbjiifapnilecob");
  GURL url = extension->GetResourceURL("blocking.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(url, contents->GetURL());

  EXPECT_EQ(true,
            content::EvalJs(contents,
                            "canSetCookie('test', 'testval', 'http://a.com')"));
}

}  // namespace extensions
