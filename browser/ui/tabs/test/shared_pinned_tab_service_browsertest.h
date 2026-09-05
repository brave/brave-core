/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_TEST_SHARED_PINNED_TAB_SERVICE_BROWSERTEST_H_
#define BRAVE_BROWSER_UI_TABS_TEST_SHARED_PINNED_TAB_SERVICE_BROWSERTEST_H_

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"

class SharedPinnedTabServiceBrowserTest : public InProcessBrowserTest {
 public:
  SharedPinnedTabServiceBrowserTest();
  ~SharedPinnedTabServiceBrowserTest() override;

  Browser* CreateNewBrowser();
  SharedPinnedTabService* GetForBrowser(Browser* browser);

  void WaitUntil(base::RepeatingCallback<bool()> condition);
  void Run();

  auto* https_server() { return https_server_.get(); }

  // InProcessBrowserTest:
  void SetUpOnMainThread() override;
  void TearDownOnMainThread() override;
  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpInProcessBrowserTestFixture() override;
  void TearDownInProcessBrowserTestFixture() override;

 private:
  base::test::ScopedFeatureList feature_list_;

  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;

  std::vector<base::WeakPtr<Browser>> browsers_;

  std::unique_ptr<base::RunLoop> run_loop_;
};

#endif  // BRAVE_BROWSER_UI_TABS_TEST_SHARED_PINNED_TAB_SERVICE_BROWSERTEST_H_
