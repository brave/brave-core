/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"

#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class BraveWalletTabHelperUnitTest : public ChromeRenderViewHostTestHarness {
 public:
  BraveWalletTabHelperUnitTest() = default;
  ~BraveWalletTabHelperUnitTest() override = default;

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    ASSERT_TRUE(web_contents());
    ON_CALL(tab_, GetContents()).WillByDefault(testing::Return(web_contents()));
    tabs::TabLookupFromWebContents::CreateForWebContents(web_contents(), &tab_);
    brave_wallet_tab_helper_ =
        std::make_unique<brave_wallet::BraveWalletTabHelper>(tab_);
  }

  void TearDown() override {
    brave_wallet_tab_helper_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  brave_wallet::BraveWalletTabHelper* brave_wallet_tab_helper() {
    return brave_wallet_tab_helper_.get();
  }

 private:
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
