/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_wallet/brave_wallet_utils.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/test/extension_test_message_listener.h"
#include "net/dns/mock_host_resolver.h"


namespace extensions {

class BraveWalletAPIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
    extension_ = extensions::ExtensionBuilder("Test").Build();
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(
      const std::string& origin, const std::string& path) {
    ui_test_utils::NavigateToURL(
        browser(),
        embedded_test_server()->GetURL(origin, path));

    return WaitForLoadStop(active_contents());
  }

 private:
  scoped_refptr<const extensions::Extension> extension_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletAPIBrowserTest, DappDetectionTest) {
  ExtensionTestMessageListener listener("brave-extension-enabled", false);
  ASSERT_TRUE(listener.WaitUntilSatisfied());
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.com", "/dapp.html"));
  base::RunLoop loop;
  SetQuitClosureForDappDetectionTest(loop.QuitClosure());
  loop.Run();
}

}  // namespace extensions
