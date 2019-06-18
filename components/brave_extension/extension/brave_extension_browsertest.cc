// /* Copyright (c) 2019 The Brave Authors. All rights reserved.
//  * This Source Code Form is subject to the terms of the Mozilla Public
//  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
//  * You can obtain one at http://mozilla.org/MPL/2.0/. */

// #include "brave/components/brave_shields/common/brave_shield_constants.h"
// #include "chrome/browser/content_settings/host_content_settings_map_factory.h"
// #include "chrome/browser/profiles/profile.h"
// #include "chrome/browser/ui/browser.h"
// #include "chrome/test/base/in_process_browser_test.h"
// #include "components/content_settings/core/browser/host_content_settings_map.h"
// #include "components/content_settings/core/common/content_settings_pattern.h"

// // npm run test -- brave_browser_tests --filter=BraveExtensionBrowserTest.*
// class BraveExtensionBrowserTest : public InProcessBrowserTest {
//  private:
//   DISALLOW_COPY_AND_ASSIGN(BraveExtensionBrowserTest);

// public:
//   using InProcessBrowserTest::InProcessBrowserTest;

//   BraveExtensionBrowserTest() {}

//   void SetUp() override {
//     InitEmbeddedTestServer();
//     InitService();
//     ExtensionBrowserTest::SetUp();
//   }
//   void SetUpOnMainThread() override {
//     ExtensionBrowserTest::SetUpOnMainThread();
//     // base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
//     //     base::BindOnce(&chrome_browser_net::SetUrlRequestMocksEnabled, true));
//     // host_resolver()->AddRule("*", "127.0.0.1");
//   }
//   void PreRunTestOnMainThread() override {
//     ExtensionBrowserTest::PreRunTestOnMainThread();
//     // WaitForHTTPSEverywhereServiceThread();
//     // ASSERT_TRUE(
//     //   g_brave_browser_process->https_everywhere_service()->IsInitialized());
//   }
//   void InitEmbeddedTestServer() {
//     brave::RegisterPathProvider();
//     base::FilePath test_data_dir;
//     base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
//     embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
//     ASSERT_TRUE(embedded_test_server()->Start());
//   }
//   // void InitService() {
//   //   brave_shields::kBraveExtensionTestId::SetIgnorePortForTest(true);
//   //   brave_shields::kBraveExtensionTestId::
//   //       SetComponentIdAndBase64PublicKeyForTest(
//   //           kBraveExtensionTestId,
//   //           kBraveExtensionTestBase64PublicKey);
//   // }

// };

// IN_PROC_BROWSER_TEST_F(BraveExtensionBrowserTest, MutationObserverTriggeredWhenDOMChanged) {
//   ASSERT_TRUE(true);
//   GURL url = embedded_test_server()->GetURL("a.com", "/mutation_observer.html");
//   ui_test_utils::NavigateToURL(browser(), url);

//   GURL iframe_url = embedded_test_server()->GetURL("www.digg.com", "/");
//   const char kIframeID[] = "test";
//   content::WebContents* contents =  browser()->tab_strip_model()->GetActiveWebContents();
//   EXPECT_TRUE(NavigateIframeToURL(contents, kIframeID, iframe_url));
//   content::RenderFrameHost* iframe_contents = ChildFrameAt(contents->GetMainFrame(), 0);
//   WaitForLoadStop(contents);
//   EXPECT_EQ(GURL("https://www.digg.com/"), iframe_contents->GetLastCommittedURL());

// }
