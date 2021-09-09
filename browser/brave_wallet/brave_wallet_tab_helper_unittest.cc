/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class BraveWalletTabHelperUnitTest : public testing::Test {
 public:
  BraveWalletTabHelperUnitTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~BraveWalletTabHelperUnitTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    profile_ = profile_manager_.CreateTestingProfile("TestProfile");
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, nullptr);
    ASSERT_TRUE(web_contents_.get());
    brave_wallet::BraveWalletTabHelper::CreateForWebContents(
        web_contents_.get());
  }

  brave_wallet::BraveWalletTabHelper* brave_wallet_tab_helper() {
    return brave_wallet::BraveWalletTabHelper::FromWebContents(
        web_contents_.get());
  }

  content::WebContents* web_contents() { return web_contents_.get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager profile_manager_;
  TestingProfile* profile_;
  std::unique_ptr<content::WebContents> web_contents_;
};

#if !defined(OS_ANDROID)
TEST_F(BraveWalletTabHelperUnitTest, GetApproveBubbleURL) {
  auto* helper = brave_wallet_tab_helper();
  ASSERT_TRUE(helper);
  ASSERT_EQ(helper->GetApproveBubbleURL(),
            GURL("chrome://wallet-panel.top-chrome/#approveTransaction"));
}
#endif

}  // namespace brave_wallet
