/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/password_manager/chrome_password_manager_client.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/core/browser/browser_autofill_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

class BraveAutofillBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  content::WebContents* PrepareWebContents(Browser* browser, const GURL& url) {
    TabStripModel* model = browser->tab_strip_model();
    auto* active_contents = model->GetActiveWebContents();
    EXPECT_TRUE(content::NavigateToURL(active_contents, url));
    EXPECT_TRUE(WaitForLoadStop(active_contents));
    EXPECT_EQ(url, active_contents->GetVisibleURL());
    return active_contents;
  }

  void TestAutofillInWindow(Browser* browser, const GURL& url, bool enabled) {
    auto* active_contents = PrepareWebContents(browser, url);
    // Logins.
    autofill::ChromeAutofillClient* autofill_client =
        autofill::ChromeAutofillClient::FromWebContentsForTesting(
            active_contents);
    EXPECT_EQ(autofill_client->IsAutocompleteEnabled(), enabled);
    // Passwords.
    ChromePasswordManagerClient* client =
        ChromePasswordManagerClient::FromWebContents(active_contents);
    EXPECT_EQ(client->IsFillingEnabled(url), enabled);
    // Other info.
    autofill::ContentAutofillDriver* cross_driver =
        autofill::ContentAutofillDriver::GetForRenderFrameHost(
            active_contents->GetPrimaryMainFrame());
    ASSERT_TRUE(cross_driver);
    EXPECT_EQ(cross_driver->GetAutofillClient().IsAutofillEnabled(), enabled);
  }
};

IN_PROC_BROWSER_TEST_F(BraveAutofillBrowserTest,
                       AutofillIsNotAllowedInPrivateWindows) {
  GURL url(
      embedded_test_server()->GetURL("example.com", "/brave_scheme_load.html"));

  // Disable autofill in private windows.
  browser()->profile()->GetPrefs()->SetBoolean(kBraveAutofillPrivateWindows,
                                               false);
  TestAutofillInWindow(browser(), url, true);
  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  TestAutofillInWindow(private_browser, url, false);

  // Enable autofill in private windows.
  browser()->profile()->GetPrefs()->SetBoolean(kBraveAutofillPrivateWindows,
                                               true);
  TestAutofillInWindow(browser(), url, true);
  TestAutofillInWindow(private_browser, url, true);

  CloseBrowserSynchronously(private_browser);
}
