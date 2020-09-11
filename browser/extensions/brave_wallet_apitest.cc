/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "brave/common/brave_wallet_constants.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "extensions/common/constants.h"
#include "extensions/test/result_catcher.h"

namespace extensions {
namespace {

class BraveWalletExtensionApiTest : public ExtensionApiTest {
 public:
  void SetUp() override {
    brave::RegisterPathProvider();
    base::PathService::Get(brave::DIR_TEST_DATA, &extension_dir_);
    extension_dir_ = extension_dir_.AppendASCII("extensions/api_test");
    ExtensionApiTest::SetUp();
  }
  void TearDown() override {
    ExtensionApiTest::TearDown();
  }
  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }
  base::FilePath extension_dir_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveExtensionWithWalletHasAccess) {
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveShieldsWithWallet"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), brave_extension_id,
      "testBasics()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveWalletAPIAvailable) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  env->SetVar("BRAVE_INFURA_PROJECT_ID", "test-project-id");
  env->SetVar("BRAVE_SERVICES_KEY", "test-brave-key");
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveWallet"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ethereum_remote_client_extension_id,
      "testBasics()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveWalletAPIKnownValuesTest) {
  GetPrefs()->SetString(kBraveWalletAES256GCMSivNonce, "yJngKDr5nCGYz7EM");
  GetPrefs()->SetString(kBraveWalletEncryptedSeed,
      "IQu5fUMbXG6E7v8ITwcIKL3TI3rst0LU1US7ZxCKpgAGgLNAN6DbCN7nMF2Eg7Kx");
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveWallet"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ethereum_remote_client_extension_id,
      "testKnownSeedValuesEndToEnd()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveWalletAPIBitGoKnownValuesTest) {
  GetPrefs()->SetString(kBraveWalletAES256GCMSivNonce, "yJngKDr5nCGYz7EM");
  GetPrefs()->SetString(kBraveWalletEncryptedSeed,
      "IQu5fUMbXG6E7v8ITwcIKL3TI3rst0LU1US7ZxCKpgAGgLNAN6DbCN7nMF2Eg7Kx");
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveWallet"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ethereum_remote_client_extension_id,
      "testKnownBitGoSeedValuesEndToEnd()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveWalletWeb3ProviderCryptoWallets) {
  GetPrefs()->SetInteger(kBraveWalletWeb3Provider,
      static_cast<int>(BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS));
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveWallet"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ethereum_remote_client_extension_id,
      "testProviderIsCryptoWallets()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveWalletWeb3ProviderMetaMask) {
  GetPrefs()->SetInteger(kBraveWalletWeb3Provider,
      static_cast<int>(BraveWalletWeb3ProviderTypes::METAMASK));
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveWallet"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ethereum_remote_client_extension_id,
      "testProviderIsMetaMask()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveWalletWeb3ProviderAsk) {
  GetPrefs()->SetInteger(kBraveWalletWeb3Provider,
      static_cast<int>(BraveWalletWeb3ProviderTypes::ASK));
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveWallet"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ethereum_remote_client_extension_id,
      "testProviderIsAsk()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveWalletWeb3ProviderNone) {
  GetPrefs()->SetInteger(kBraveWalletWeb3Provider,
      static_cast<int>(BraveWalletWeb3ProviderTypes::NONE));
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveWallet"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ethereum_remote_client_extension_id,
      "testProviderIsNone()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveWalletAPINotAvailable) {
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("notBraveWallet"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveShieldsDappDetectionWhenDefault) {
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveShieldsWithWallet"));
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), brave_extension_id, "testDappCheck()"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveShieldsDappDetectionWhenAsk) {
  GetPrefs()->SetInteger(kBraveWalletWeb3Provider,
      static_cast<int>(BraveWalletWeb3ProviderTypes::ASK));
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveShieldsWithWallet"));
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), brave_extension_id, "testDappCheck()"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveShieldsNoDappDetectionWhenNone) {
  GetPrefs()->SetInteger(kBraveWalletWeb3Provider,
      static_cast<int>(BraveWalletWeb3ProviderTypes::NONE));
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveShieldsWithWallet"));
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), brave_extension_id, "testNoDappCheck()"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveShieldsNoDappDetectionWhenMetaMask) {
  GetPrefs()->SetInteger(kBraveWalletWeb3Provider,
      static_cast<int>(BraveWalletWeb3ProviderTypes::METAMASK));
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveShieldsWithWallet"));
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), brave_extension_id, "testNoDappCheck()"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveShieldsNoDappDetectionWhenCryptoWallets) {
  GetPrefs()->SetInteger(kBraveWalletWeb3Provider,
      static_cast<int>(BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS));
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveShieldsWithWallet"));
  LoadExtension(extension_dir_.AppendASCII("braveWallet"));
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), brave_extension_id, "testNoDappCheck()"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(BraveWalletExtensionApiTest,
    BraveShieldsDappDetectionWhenCryptoWalletsNotReady) {
  GetPrefs()->SetInteger(kBraveWalletWeb3Provider,
      static_cast<int>(BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS));
  ResultCatcher catcher;
  const Extension* extension =
    LoadExtension(extension_dir_.AppendASCII("braveShieldsWithWallet"));
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), brave_extension_id, "testDappCheck()"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

}  // namespace
}  // namespace extensions
