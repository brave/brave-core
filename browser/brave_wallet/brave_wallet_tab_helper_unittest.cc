/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"

#include "chrome/test/base/browser_with_test_window_test.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class BraveWalletTabHelperUnitTest : public BrowserWithTestWindowTest {
 public:
  BraveWalletTabHelperUnitTest() = default;
  ~BraveWalletTabHelperUnitTest() override = default;

  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
    ASSERT_TRUE(web_contents_.get());
    ON_CALL(tab_, GetContents())
        .WillByDefault(testing::Return(web_contents_.get()));
    tabs::TabLookupFromWebContents::CreateForWebContents(web_contents_.get(),
                                                         &tab_);
    brave_wallet_tab_helper_ =
        std::make_unique<brave_wallet::BraveWalletTabHelper>(tab_);
  }

  void TearDown() override {
    brave_wallet_tab_helper_.reset();
    web_contents_.reset();
    BrowserWithTestWindowTest::TearDown();
  }

  brave_wallet::BraveWalletTabHelper* brave_wallet_tab_helper() {
    return brave_wallet_tab_helper_.get();
  }

  content::WebContents* web_contents() { return web_contents_.get(); }

 private:
  std::unique_ptr<content::WebContents> web_contents_;
  tabs::MockTabInterface tab_;
  std::unique_ptr<brave_wallet::BraveWalletTabHelper> brave_wallet_tab_helper_;
};

TEST_F(BraveWalletTabHelperUnitTest, GetApproveBubbleURL) {
  auto* helper = brave_wallet_tab_helper();
  ASSERT_TRUE(helper);
  ASSERT_EQ(helper->GetApproveBubbleURL(),
            GURL("chrome://wallet-panel.top-chrome/#approveTransaction"));
}

}  // namespace brave_wallet
