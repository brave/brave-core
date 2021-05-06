// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"

class WalletPanelUIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    webui_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(browser()->profile()));

    webui_contents_->GetController().LoadURLWithParams(
        content::NavigationController::LoadURLParams(
            GURL(kBraveUIWalletPanelURL)));

    // Finish loading after initializing.
    ASSERT_TRUE(content::WaitForLoadStop(webui_contents_.get()));
  }

  void TearDownOnMainThread() override { webui_contents_.reset(); }

  void AppendTab(std::string url) {
    chrome::AddTabAt(browser(), GURL(url), -1, true);
  }

  content::WebContents* GetActiveTab() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  WalletPanelUI* GetWebUIController() {
    return webui_contents_->GetWebUI()
        ->GetController()
        ->template GetAs<WalletPanelUI>();
  }

 protected:
  std::unique_ptr<content::WebContents> webui_contents_;

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(WalletPanelUIBrowserTest, InitialUIRendered) {
  const std::string wallet_panel_js = "!!document.querySelector('#mountPoint')";
  bool exists = content::EvalJs(webui_contents_.get(), wallet_panel_js,
                                content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                                ISOLATED_WORLD_ID_CHROME_INTERNAL)
                    .ExtractBool();
  ASSERT_TRUE(exists);
}
