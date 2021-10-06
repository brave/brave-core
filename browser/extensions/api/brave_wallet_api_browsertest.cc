/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/scoped_observation.h"
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/test/extension_test_message_listener.h"
#include "net/dns/mock_host_resolver.h"

namespace extensions {

class BraveWalletAPIBrowserTest : public InProcessBrowserTest {
 public:
  BraveWalletAPIBrowserTest() {}

  void WaitForBraveExtensionAdded() {
    // Brave extension must be loaded, otherwise dapp detection events
    // could be missed from a race condition.
    ExtensionTestMessageListener extension_listener("brave-extension-enabled",
        false);
    ASSERT_TRUE(extension_listener.WaitUntilSatisfied());
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

  void AddFakeMetaMaskExtension(bool is_update) {
    DictionaryBuilder manifest;
    manifest.Set("name", "ext")
        .Set("version", "0.1")
        .Set("manifest_version", 2);
    extension_ = extensions::ExtensionBuilder()
                     .SetManifest(manifest.Build())
                     .SetID(metamask_extension_id)
                     .Build();
    ASSERT_TRUE(extension_);
    if (!is_update) {
      ExtensionRegistry::Get(browser()->profile())->AddEnabled(
          extension_.get());
    }
    ExtensionRegistry::Get(browser()->profile())->TriggerOnInstalled(
        extension_.get(), is_update);
    if (!is_update) {
      ExtensionRegistry::Get(browser()->profile())->AddReady(extension_.get());
    }
  }

  void RemoveFakeMetaMaskExtension() {
    ExtensionRegistry::Get(browser()->profile())->RemoveReady(
        metamask_extension_id);
    ExtensionRegistry::Get(browser()->profile())->RemoveEnabled(
        metamask_extension_id);
    ExtensionRegistry::Get(browser()->profile())->TriggerOnUninstalled(
        extension_.get(), extensions::UNINSTALL_REASON_FOR_TESTING);
  }

  ~BraveWalletAPIBrowserTest() override {
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const std::string& origin,
      const std::string& path) {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL(origin, path)));
    return WaitForLoadStop(active_contents());
  }

 private:
  scoped_refptr<const extensions::Extension> extension_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletAPIBrowserTest,
    FakeInstallMetaMask) {
  if (brave_wallet::IsNativeWalletEnabled()) {
    browser()->profile()->GetPrefs()->SetInteger(
        kBraveWalletWeb3Provider,
        static_cast<int>(brave_wallet::Web3ProviderTypes::ASK));
  }
  WaitForBraveExtensionAdded();
  AddFakeMetaMaskExtension(false);
  // Should auto select MetaMask
  auto provider = static_cast<brave_wallet::Web3ProviderTypes>(
      browser()->profile()->GetPrefs()->GetInteger(kBraveWalletWeb3Provider));
  ASSERT_EQ(provider, brave_wallet::Web3ProviderTypes::METAMASK);
}

IN_PROC_BROWSER_TEST_F(BraveWalletAPIBrowserTest,
    FakeUninstallMetaMask) {
  WaitForBraveExtensionAdded();
  AddFakeMetaMaskExtension(false);
  RemoveFakeMetaMaskExtension();
  // Should revert back to Ask
  auto provider = static_cast<brave_wallet::Web3ProviderTypes>(
      browser()->profile()->GetPrefs()->GetInteger(kBraveWalletWeb3Provider));
  if (brave_wallet::IsNativeWalletEnabled()) {
    ASSERT_EQ(provider, brave_wallet::Web3ProviderTypes::BRAVE_WALLET);
  } else {
    ASSERT_EQ(provider, brave_wallet::Web3ProviderTypes::CRYPTO_WALLETS);
  }
}

IN_PROC_BROWSER_TEST_F(BraveWalletAPIBrowserTest,
    UpdatesDoNotChangeSettings) {
  WaitForBraveExtensionAdded();
  // User installs MetaMask
  AddFakeMetaMaskExtension(false);
  // Then if the user explicitly manually sets it to Crypto Wallets
  browser()->profile()->GetPrefs()->SetInteger(
      kBraveWalletWeb3Provider,
      static_cast<int>(brave_wallet::Web3ProviderTypes::CRYPTO_WALLETS));
  // Then the user updates MetaMask
  AddFakeMetaMaskExtension(true);
  // It should not toggle the setting
  auto provider = static_cast<brave_wallet::Web3ProviderTypes>(
      browser()->profile()->GetPrefs()->GetInteger(kBraveWalletWeb3Provider));
  ASSERT_EQ(provider, brave_wallet::Web3ProviderTypes::CRYPTO_WALLETS);
}

}  // namespace extensions
