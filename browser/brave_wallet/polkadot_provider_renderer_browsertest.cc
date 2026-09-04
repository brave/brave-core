/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace brave_wallet {

namespace {

constexpr char kCheckPolkadotProviderScript[] =
    "!!window.injectedWeb3 && !!window.injectedWeb3['brave-wallet']";

}  // namespace

class PolkadotProviderRendererTest : public InProcessBrowserTest {
 public:
  PolkadotProviderRendererTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "true"}}}},
        {});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // The provider is only injected once a wallet exists, so create one before
    // the navigation below.
    ASSERT_TRUE(GetKeyringService()->RestoreWalletSync(
        kMnemonicScarePiece, kTestWalletPassword, false));
    ASSERT_TRUE(test_server_handle_ =
                    embedded_test_server()->StartAndReturnHandle());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("/empty.html")));
  }

  content::WebContents* web_contents(Browser* browser) const {
    return browser->tab_strip_model()->GetActiveWebContents();
  }

  void ReloadAndWaitForLoadStop(Browser* browser) {
    chrome::Reload(browser, WindowOpenDisposition::CURRENT_TAB);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents(browser)));
  }

  KeyringService* GetKeyringService() {
    return BraveWalletServiceFactory::GetServiceForContext(
               browser()->GetProfile())
        ->keyring_service();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
};

class PolkadotProviderDisabledRendererTest
    : public PolkadotProviderRendererTest {
 public:
  PolkadotProviderDisabledRendererTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "false"}}}},
        {});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, AttachIfWalletCreated) {
  auto result = content::EvalJs(web_contents(browser()),
                                kCheckPolkadotProviderScript);
  EXPECT_EQ(base::Value(true), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, Version) {
  auto result = content::EvalJs(web_contents(browser()),
                                "window.injectedWeb3['brave-wallet'].version");
  EXPECT_EQ(base::Value("1.0.0"), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderDisabledRendererTest,
                       NotAttached_FeatureDisabled) {
  ReloadAndWaitForLoadStop(browser());

  auto result = content::EvalJs(web_contents(browser()),
                                kCheckPolkadotProviderScript);
  EXPECT_EQ(base::Value(false), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest,
                       DoNotAttachIfNoWalletCreated) {
  GetKeyringService()->Reset(false);

  ReloadAndWaitForLoadStop(browser());

  auto result =
      content::EvalJs(web_contents(browser()), "!!window.injectedWeb3");
  EXPECT_EQ(base::Value(false), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, Incognito) {
  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      private_browser, embedded_test_server()->GetURL("/empty.html")));

  auto result = content::EvalJs(web_contents(private_browser),
                                kCheckPolkadotProviderScript);
  EXPECT_EQ(base::Value(false), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, NonWritableEntry) {
  auto result = content::EvalJs(
      web_contents(browser()),
      R"(window.injectedWeb3['brave-wallet'] = ['test'];
         window.injectedWeb3['brave-wallet'].version)");
  EXPECT_EQ(base::Value("1.0.0"), result);
}

// `window.injectedWeb3` is shared with every other injecting wallet, and
// @polkadot/extension-inject reassigns the property itself before adding its
// own key, from strict mode. Neither may be broken by our entry.
IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, OtherWalletCanInject) {
  auto result = content::EvalJs(web_contents(browser()),
                                R"('use strict';
         window.injectedWeb3 = window.injectedWeb3 || {};
         window.injectedWeb3['other-wallet'] = { version: '1' };
         !!window.injectedWeb3['brave-wallet'] &&
             !!window.injectedWeb3['other-wallet'])");
  EXPECT_EQ(base::Value(true), result);
}

}  // namespace brave_wallet
