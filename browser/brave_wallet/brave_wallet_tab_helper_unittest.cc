/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"

#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(OS_ANDROID)
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/hid/hid_chooser_controller.h"
#include "chrome/grit/generated_resources.h"
#include "components/permissions/chooser_title_util.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#endif

#if !defined(OS_ANDROID)
namespace {
std::u16string BraveCreateTitleLabel() {
  auto wallet_title = l10n_util::GetStringUTF16(IDS_BRAVE_WALLET);
  return l10n_util::GetStringFUTF16(IDS_HID_CHOOSER_PROMPT_ORIGIN,
                                    wallet_title);
}

std::u16string GetHIDTitle(content::WebContents* content, const GURL& url) {
  content::WebContentsTester::For(content)->NavigateAndCommit(
      url, ui::PAGE_TRANSITION_LINK);
  std::vector<blink::mojom::HidDeviceFilterPtr> filters;
  auto hid_chooser_controller = std::make_unique<HidChooserController>(
      content->GetMainFrame(), std::move(filters), base::DoNothing());
  return hid_chooser_controller->GetTitle();
}
}  // namespace
#endif

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
  raw_ptr<TestingProfile> profile_ = nullptr;
  // Needed to ensure we don't end up creating actual RenderViewHosts
  // and RenderProcessHosts.
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  std::unique_ptr<content::WebContents> web_contents_;
};

#if !defined(OS_ANDROID)
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

#endif

}  // namespace brave_wallet
