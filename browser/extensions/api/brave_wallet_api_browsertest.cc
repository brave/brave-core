/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/scoped_observer.h"
#include "brave/browser/infobars/crypto_wallets_infobar_delegate.h"
#include "brave/common/brave_paths.h"
#include "brave/common/brave_wallet_constants.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/test/extension_test_message_listener.h"
#include "net/dns/mock_host_resolver.h"

using namespace infobars;  // NOLINT

namespace extensions {

class BraveWalletAPIBrowserTest : public InProcessBrowserTest,
    public InfoBarManager::Observer {
 public:
  BraveWalletAPIBrowserTest() : infobar_observer_(this),
      infobar_added_(false) {
  }

  void WaitForBraveExtensionAdded() {
    // Brave extension must be loaded, otherwise dapp detection events
    // could be missed from a race condition.
    ExtensionTestMessageListener extension_listener("brave-extension-enabled",
        false);
    ASSERT_TRUE(extension_listener.WaitUntilSatisfied());
  }

  void WaitForCryptoWalletsInfobarAdded() {
    if (infobar_added_) {
      return;
    }
    infobar_added_run_loop_ = std::make_unique<base::RunLoop>();
    infobar_added_run_loop_->Run();
  }

  void WaitForTabCount(int expected) {
    while (browser()->tab_strip_model()->count() != expected)
      base::RunLoop().RunUntilIdle();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void AddFakeMetaMaskExtension() {
    DictionaryBuilder manifest;
    manifest.Set("name", "ext")
        .Set("version", "0.1")
        .Set("manifest_version", 2);
    extension_ = extensions::ExtensionBuilder()
                     .SetManifest(manifest.Build())
                     .SetID(metamask_extension_id)
                     .Build();
    ASSERT_TRUE(extension_);
    ExtensionRegistry::Get(browser()->profile())->AddReady(extension_.get());
  }


  ~BraveWalletAPIBrowserTest() override {
  }

  void AddInfoBarObserver(InfoBarService* infobar_service) {
    infobar_observer_.Add(infobar_service);
  }

  void RemoveInfoBarObserver(InfoBarService* infobar_service) {
    infobar_observer_.Remove(infobar_service);
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void CryptoWalletsInfoBarAccept(int expected_buttons) {
    InfoBarService* infobar_service =
        InfoBarService::FromWebContents(active_contents());
    for (size_t i = 0; i < infobar_service->infobar_count(); i++) {
      InfoBarDelegate* delegate =
          infobar_service->infobar_at(i)->delegate();
      if (delegate->GetIdentifier() ==
              InfoBarDelegate::CRYPTO_WALLETS_INFOBAR_DELEGATE) {
        ConfirmInfoBarDelegate* confirm_delegate =
            delegate->AsConfirmInfoBarDelegate();
        // Only the OK button should be present
        ASSERT_EQ(confirm_delegate->GetButtons(), expected_buttons);
        confirm_delegate->Accept();
      }
    }
  }

  void CryptoWalletsInfoBarCancel(int expected_buttons) {
    InfoBarService* infobar_service =
        InfoBarService::FromWebContents(active_contents());
    for (size_t i = 0; i < infobar_service->infobar_count(); i++) {
      InfoBarDelegate* delegate =
          infobar_service->infobar_at(i)->delegate();
      if (delegate->GetIdentifier() ==
              InfoBarDelegate::CRYPTO_WALLETS_INFOBAR_DELEGATE) {
        ConfirmInfoBarDelegate* confirm_delegate =
            delegate->AsConfirmInfoBarDelegate();
        // Only the OK button should be present
        ASSERT_EQ(confirm_delegate->GetButtons(),
            expected_buttons);
        confirm_delegate->Cancel();
      }
    }
  }

  bool NavigateToURLUntilLoadStop(const std::string& origin,
      const std::string& path) {
    ui_test_utils::NavigateToURL(browser(),
        embedded_test_server()->GetURL(origin, path));
    return WaitForLoadStop(active_contents());
  }

 private:
  scoped_refptr<const extensions::Extension> extension_;
  ScopedObserver<InfoBarManager, InfoBarManager::Observer>
      infobar_observer_;
  bool infobar_added_;
  std::unique_ptr<base::RunLoop> infobar_added_run_loop_;

  // InfoBarManager::Observer:
  void OnInfoBarAdded(InfoBar* infobar) override {
    if (infobar_added_run_loop_ &&
        infobar->delegate()->GetIdentifier() ==
            InfoBarDelegate::CRYPTO_WALLETS_INFOBAR_DELEGATE) {
      infobar_added_ = true;
      infobar_added_run_loop_->Quit();
    }
  }
};

IN_PROC_BROWSER_TEST_F(BraveWalletAPIBrowserTest, DappDetectionTestAccept) {
  WaitForBraveExtensionAdded();
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(active_contents());
  AddInfoBarObserver(infobar_service);
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.com", "/dapp.html"));
  WaitForCryptoWalletsInfobarAdded();
  // Pref for Wallet should still be ask by default
  auto provider = static_cast<BraveWalletWeb3ProviderTypes>(
      browser()->profile()->GetPrefs()->GetInteger(kBraveWalletWeb3Provider));
  ASSERT_EQ(provider, BraveWalletWeb3ProviderTypes::ASK);
  CryptoWalletsInfoBarAccept(ConfirmInfoBarDelegate::BUTTON_OK);
  WaitForTabCount(2);
  RemoveInfoBarObserver(infobar_service);
}

IN_PROC_BROWSER_TEST_F(BraveWalletAPIBrowserTest,
    DappDetectionWithMetaMaskTestAccept) {
  WaitForBraveExtensionAdded();
  AddFakeMetaMaskExtension();
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(active_contents());
  AddInfoBarObserver(infobar_service);
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.com", "/dapp.html"));
  WaitForCryptoWalletsInfobarAdded();
  CryptoWalletsInfoBarAccept(ConfirmInfoBarDelegate::BUTTON_OK |
      ConfirmInfoBarDelegate::BUTTON_CANCEL);
  WaitForTabCount(2);
  RemoveInfoBarObserver(infobar_service);
}

IN_PROC_BROWSER_TEST_F(BraveWalletAPIBrowserTest,
    DappDetectionWithMetaMaskTestCancel) {
  WaitForBraveExtensionAdded();
  AddFakeMetaMaskExtension();
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(active_contents());
  AddInfoBarObserver(infobar_service);
  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.com", "/dapp.html"));
  WaitForCryptoWalletsInfobarAdded();
  CryptoWalletsInfoBarCancel(ConfirmInfoBarDelegate::BUTTON_OK |
      ConfirmInfoBarDelegate::BUTTON_CANCEL);
  auto provider = static_cast<BraveWalletWeb3ProviderTypes>(
      browser()->profile()->GetPrefs()->GetInteger(kBraveWalletWeb3Provider));
  ASSERT_EQ(provider, BraveWalletWeb3ProviderTypes::METAMASK);
  RemoveInfoBarObserver(infobar_service);
}


}  // namespace extensions
