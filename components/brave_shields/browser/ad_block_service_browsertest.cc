/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/thread_test_helper.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"


const char kAdsPage[] = "/blocking.html";
const char kNoAdsPage[] = "/no_blocking.html";

class AdBlockServiceTest : public InProcessBrowserTest {
public:
  AdBlockServiceTest() {}

  void SetUp() override {
    InitEmbeddedTestServer();
    InitAdBlock();
    InProcessBrowserTest::SetUp();
  }

  void PreRunTestOnMainThread() override {
    InProcessBrowserTest::PreRunTestOnMainThread();
    WaitForAdBlockServiceThread();
    ASSERT_TRUE(
      brave_shields::AdBlockService::GetInstance()->IsInitialized());
  }

  void InitEmbeddedTestServer() {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void InitAdBlock() {
    brave_shields::AdBlockService::g_ad_block_url =
        embedded_test_server()->GetURL("adblock-data/3/ABPFilterParserData.dat");
  }

  void WaitForAdBlockServiceThread() {
    scoped_refptr<base::ThreadTestHelper> io_helper(
        new base::ThreadTestHelper(
            brave_shields::AdBlockService::GetInstance()->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }
};

// Load a page with an ad image, and make sure it is blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, AdsGetBlocked) {
  GURL url = embedded_test_server()->GetURL(kAdsPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  bool img_loaded;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(imgLoaded())",
      &img_loaded));
  EXPECT_TRUE(!img_loaded);
}

// Load a page with an image which is not an ad, and make sure it is NOT blocked.
IN_PROC_BROWSER_TEST_F(AdBlockServiceTest, NotAdsDoNotGetBlocked) {
  GURL url = embedded_test_server()->GetURL(kNoAdsPage);
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ(url, contents->GetURL());

  bool img_loaded;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      contents,
      "window.domAutomationController.send(imgLoaded())",
      &img_loaded));
  EXPECT_TRUE(img_loaded);
}
