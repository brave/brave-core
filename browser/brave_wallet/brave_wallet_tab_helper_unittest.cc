/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"

#include <vector>

#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/ui/hid/hid_chooser_controller.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/chooser_title_util.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/test/web_contents_tester.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
std::u16string BraveCreateTitleLabel() {
  auto wallet_title =
      brave_l10n::GetLocalizedResourceUTF16String(IDS_BRAVE_WALLET);
  return l10n_util::GetStringFUTF16(IDS_HID_CHOOSER_PROMPT_ORIGIN,
                                    wallet_title);
}

std::u16string GetHIDTitle(content::WebContents* content, const GURL& url) {
  content::WebContentsTester::For(content)->NavigateAndCommit(
      url, ui::PAGE_TRANSITION_LINK);
  std::vector<blink::mojom::HidDeviceFilterPtr> filters;
  std::vector<blink::mojom::HidDeviceFilterPtr> exclusion_filters;
  auto hid_chooser_controller = std::make_unique<HidChooserController>(
      content->GetPrimaryMainFrame(), std::move(filters),
      std::move(exclusion_filters), base::DoNothing());
  return hid_chooser_controller->GetTitle();
}
}  // namespace

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
    brave_wallet::BraveWalletTabHelper::CreateForWebContents(
        web_contents_.get());
  }

  void TearDown() override {
    web_contents_.reset();
    BrowserWithTestWindowTest::TearDown();
  }

  brave_wallet::BraveWalletTabHelper* brave_wallet_tab_helper() {
    return brave_wallet::BraveWalletTabHelper::FromWebContents(
        web_contents_.get());
  }

  content::WebContents* web_contents() { return web_contents_.get(); }

 private:
  std::unique_ptr<content::WebContents> web_contents_;
};

TEST_F(BraveWalletTabHelperUnitTest, GetApproveBubbleURL) {
  auto* helper = brave_wallet_tab_helper();
  ASSERT_TRUE(helper);
  ASSERT_EQ(helper->GetApproveBubbleURL(),
            GURL("chrome://wallet-panel.top-chrome/#approveTransaction"));
}

TEST_F(BraveWalletTabHelperUnitTest, ChooserTitle) {
  auto wallet_label = BraveCreateTitleLabel();
  EXPECT_EQ(GetHIDTitle(web_contents(), GURL(kBraveUIWalletPanelURL)),
            wallet_label);
  EXPECT_EQ(GetHIDTitle(web_contents(), GURL(kBraveUIWalletPageURL)),
            wallet_label);
  EXPECT_NE(GetHIDTitle(web_contents(), GURL("a.com")), wallet_label);
}

TEST_F(BraveWalletTabHelperUnitTest, SolanaConnectedAccount) {
  auto* helper = brave_wallet_tab_helper();
  ASSERT_TRUE(helper);
  ASSERT_TRUE(helper->solana_connected_accounts_.empty());

  content::GlobalRenderFrameHostId rfh_id1(1, 1);
  content::GlobalRenderFrameHostId rfh_id2(1, 2);
  const char account1[] = "account1";
  const char account2[] = "account2";
  const char account3[] = "account3";

  for (const auto& id : {rfh_id1, rfh_id2}) {
    SCOPED_TRACE(id);
    ASSERT_FALSE(helper->IsSolanaAccountConnected(id, account1));
    ASSERT_FALSE(helper->IsSolanaAccountConnected(id, account2));
    ASSERT_FALSE(helper->IsSolanaAccountConnected(id, account3));
  }

  helper->AddSolanaConnectedAccount(rfh_id1, account1);
  helper->AddSolanaConnectedAccount(rfh_id2, account2);
  EXPECT_TRUE(helper->IsSolanaAccountConnected(rfh_id1, account1));
  EXPECT_FALSE(helper->IsSolanaAccountConnected(rfh_id2, account1));
  EXPECT_FALSE(helper->IsSolanaAccountConnected(rfh_id1, account2));
  EXPECT_TRUE(helper->IsSolanaAccountConnected(rfh_id2, account2));
  ASSERT_EQ(helper->solana_connected_accounts_[rfh_id1].size(), 1u);
  ASSERT_EQ(helper->solana_connected_accounts_[rfh_id2].size(), 1u);
  ASSERT_EQ(helper->solana_connected_accounts_.size(), 2u);

  // remove non-existing account3
  helper->RemoveSolanaConnectedAccount(rfh_id1, account3);
  ASSERT_EQ(helper->solana_connected_accounts_[rfh_id1].size(), 1u);
  EXPECT_TRUE(helper->IsSolanaAccountConnected(rfh_id1, account1));
  EXPECT_FALSE(helper->IsSolanaAccountConnected(rfh_id1, account2));

  // add account2 and remove account2 for rfh_id1
  helper->AddSolanaConnectedAccount(rfh_id1, account2);
  EXPECT_TRUE(helper->IsSolanaAccountConnected(rfh_id1, account2));
  helper->RemoveSolanaConnectedAccount(rfh_id1, account2);
  EXPECT_FALSE(helper->IsSolanaAccountConnected(rfh_id1, account2));

  // clear set from rfh_id1
  helper->ClearSolanaConnectedAccounts(rfh_id1);
  EXPECT_FALSE(helper->IsSolanaAccountConnected(rfh_id1, account1));
  EXPECT_FALSE(helper->IsSolanaAccountConnected(rfh_id1, account2));
  EXPECT_FALSE(helper->IsSolanaAccountConnected(rfh_id1, account3));
  ASSERT_EQ(helper->solana_connected_accounts_.size(), 1u);
}

}  // namespace brave_wallet
