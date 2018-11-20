/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_extension_functional_test.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/net/url_request_mock_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"

using BraveExtensionProviderTest = extensions::ExtensionFunctionalTest;

namespace extensions {

IN_PROC_BROWSER_TEST_F(BraveExtensionProviderTest, BlacklistExtension) {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  const extensions::Extension* extension =
    InstallExtension(test_data_dir.AppendASCII("should-be-blocked-extension"), 0);
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

IN_PROC_BROWSER_TEST_F(BraveExtensionProviderTest, PDFJSInstalls) {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  InstallExtensionSilently(extension_service(),
      test_data_dir.AppendASCII("pdfjs.crx"));

  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  GURL url = embedded_test_server()->GetURL("/test.pdf");
  ui_test_utils::NavigateToURL(browser(), url);
  ASSERT_TRUE(WaitForLoadStop(contents));

  // Test.pdf is just a PDF file for an icon with title test.ico
  base::string16 expected_title(base::ASCIIToUTF16("test.ico"));
    content::TitleWatcher title_watcher(
        browser()->tab_strip_model()->GetActiveWebContents(), expected_title);
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_EQ(expected_title, title_watcher.WaitAndGetTitle());

  // Make sure pdfjs embed exists
  bool pdfjs_exists;
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetWebContentsAt(0),
      "window.domAutomationController.send("
        "!!document.body.querySelector(\"embed[src$='.pdf']\"))", &pdfjs_exists));
  ASSERT_TRUE(pdfjs_exists);
}

}  // namespace extnesions
