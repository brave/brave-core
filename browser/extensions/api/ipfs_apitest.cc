/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "extensions/common/constants.h"
#include "extensions/test/result_catcher.h"

namespace extensions {
namespace {

// Actual config option string doesn't matter for tests
const char ipfs_config[] = "{ \"Identity\": {} }";

class IpfsExtensionApiTest : public ExtensionApiTest {
 public:
  IpfsExtensionApiTest() {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
  }
  void SetUp() override {
    brave::RegisterPathProvider();
    base::PathService::Get(brave::DIR_TEST_DATA, &extension_dir_);
    extension_dir_ = extension_dir_.AppendASCII("extensions/api_test");
    ExtensionApiTest::SetUp();
  }
  void WriteConfigToFile() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ipfs::IpfsService* service =
        ipfs::IpfsServiceFactory::GetInstance()->GetForContext(profile());
    base::FilePath path = service->GetConfigFilePath();
    ASSERT_TRUE(base::CreateDirectory(path.DirName()));
    ASSERT_TRUE(base::WriteFile(path, ipfs_config));
  }
  void TearDown() override { ExtensionApiTest::TearDown(); }
  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }
  base::FilePath extension_dir_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, IpfsCompanionHasAcces) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("ipfsCompanion"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id, "testBasics()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, ExecutableAvailChangeIsReflected) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("ipfsCompanion"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id,
      "executableAvailableChangeIsReflected(false)"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;

  GetPrefs()->SetFilePath(kIPFSBinaryPath,
                          base::FilePath(FILE_PATH_LITERAL("some_path")));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id,
      "executableAvailableChangeIsReflected(true)"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, ResolveMethodChangeIsReflected) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("ipfsCompanion"));
  GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id,
      "resolveMethodChangeIsReflected('local')"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;

  GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_DISABLED));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id,
      "resolveMethodChangeIsReflected('disabled')"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;

  GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id,
      "resolveMethodChangeIsReflected('gateway')"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, GetConfig) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("ipfsCompanion"));
  ASSERT_TRUE(extension);

  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id,
      "getConfig(false, '')"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;

  WriteConfigToFile();
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id,
      std::string("getConfig(true, '") + ipfs_config + "')"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

// No great way to test launch and shutdown succeeding easily, so at least
// just make sure the API call works. IpfsService::SetAllowIpfsLaunchForTest
// is used to short-circuit the launch and shutdown process.
IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, LaunchShutdownSuccess) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("ipfsCompanion"));
  ASSERT_TRUE(extension);
  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetInstance()->GetForContext(
          browser()->profile());
  ASSERT_TRUE(service);

  GetPrefs()->SetFilePath(kIPFSBinaryPath,
                          base::FilePath(FILE_PATH_LITERAL("some_path")));
  service->SetAllowIpfsLaunchForTest(true);

  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id, "launchSuccess()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;

  service->SetAllowIpfsLaunchForTest(false);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id, "shutdownSuccess()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, LaunchFailWhenNotInstalled) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("ipfsCompanion"));
  ASSERT_TRUE(extension);
  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetInstance()->GetForContext(
          browser()->profile());
  ASSERT_TRUE(service);

  service->SetAllowIpfsLaunchForTest(true);
  GetPrefs()->SetFilePath(kIPFSBinaryPath, base::FilePath());
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id, "launchFail()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, IpfsAPINotAvailable) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("notIpfsCompanion"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, ResolveIPFSURIMatches) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("ipfsCompanion"));
  ASSERT_TRUE(extension);
  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetInstance()->GetForContext(
          browser()->profile());
  ASSERT_TRUE(service);

  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id,
      "resolveIPFSURIMatches("
      "'ipfs://bafybeifk6th5qhox7pffjqjerbjxkpmsmufdcswdgacnmyv3fn53z2wgwe',"
      "'https://bafybeifk6th5qhox7pffjqjerbjxkpmsmufdcswdgacnmyv3fn53z2wgwe"
      ".ipfs.dweb.link/')"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, IpfsPermissionAPIAccess) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("ExtensionWithIpfsPermission"));
  ASSERT_TRUE(extension);
  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_persmission_extension_id, "testBasics()"));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, GetSettings) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("ipfsCompanion"));
  ASSERT_TRUE(extension);
  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetInstance()->GetForContext(
          browser()->profile());
  ASSERT_TRUE(service);

  service->SetAllowIpfsLaunchForTest(true);
  GetPrefs()->SetBoolean(kIPFSAutoRedirectToConfiguredGateway, true);
  GetPrefs()->SetBoolean(kIPFSAutoFallbackToGateway, true);
  GetPrefs()->SetInteger(kIpfsStorageMax, 2);

  GetPrefs()->SetString(kIPFSPublicGatewayAddress, "https://a.b");
  GetPrefs()->SetString(kIPFSPublicNFTGatewayAddress, "https://a.b.c");
  GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK));

  std::string expected_json =
      "{\"gateway_auto_fallback_enabled\": true, "
      "\"auto_redirect_to_configured_gateway\": true, \"storage_max\": 2, "
      "\"gateway_url\": \"https://a.b\", \"nft_gateway_url\": "
      "\"https://a.b.c\", \"resolve_method\": \"ask\"}";

  ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
      browser()->profile(), ipfs_companion_extension_id,
      base::StringPrintf("getSettings('%s')", expected_json.c_str())));
  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(IpfsExtensionApiTest, SetSettings) {
  ResultCatcher catcher;
  const Extension* extension =
      LoadExtension(extension_dir_.AppendASCII("ipfsCompanion"));
  ASSERT_TRUE(extension);
  ipfs::IpfsService* service =
      ipfs::IpfsServiceFactory::GetInstance()->GetForContext(
          browser()->profile());
  ASSERT_TRUE(service);

  service->SetAllowIpfsLaunchForTest(true);

  {
    ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
        browser()->profile(), ipfs_companion_extension_id,
        "setPublicGateway('https://ipfs.io/')"));
    ASSERT_TRUE(catcher.GetNextResult()) << message_;
    EXPECT_EQ(GetPrefs()->GetString(kIPFSPublicGatewayAddress),
              "https://ipfs.io/");
  }

  {
    ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
        browser()->profile(), ipfs_companion_extension_id,
        "setPublicNFTGateway('https://ipfs.io/')"));
    ASSERT_TRUE(catcher.GetNextResult()) << message_;
    EXPECT_EQ(GetPrefs()->GetString(kIPFSPublicNFTGatewayAddress),
              "https://ipfs.io/");
  }

  {
    ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
        browser()->profile(), ipfs_companion_extension_id,
        "setResolveMethod('ask')"));
    ASSERT_TRUE(catcher.GetNextResult()) << message_;
    EXPECT_EQ(GetPrefs()->GetInteger(kIPFSResolveMethod),
              static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK));

    ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
        browser()->profile(), ipfs_companion_extension_id,
        "setResolveMethod('local')"));
    ASSERT_TRUE(catcher.GetNextResult()) << message_;
    EXPECT_EQ(GetPrefs()->GetInteger(kIPFSResolveMethod),
              static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));

    ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
        browser()->profile(), ipfs_companion_extension_id,
        "setResolveMethod('gateway')"));
    ASSERT_TRUE(catcher.GetNextResult()) << message_;
    EXPECT_EQ(GetPrefs()->GetInteger(kIPFSResolveMethod),
              static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));

    ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
        browser()->profile(), ipfs_companion_extension_id,
        "setResolveMethod('disabled')"));
    ASSERT_TRUE(catcher.GetNextResult()) << message_;
    EXPECT_EQ(GetPrefs()->GetInteger(kIPFSResolveMethod),
              static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_DISABLED));
  }

  {
    ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
        browser()->profile(), ipfs_companion_extension_id,
        "setAutoRedirectToConfiguredGateway(true)"));
    ASSERT_TRUE(catcher.GetNextResult()) << message_;
    EXPECT_EQ(GetPrefs()->GetBoolean(kIPFSAutoRedirectToConfiguredGateway),
              true);
  }

  {
    ASSERT_TRUE(browsertest_util::ExecuteScriptInBackgroundPageNoWait(
        browser()->profile(), ipfs_companion_extension_id,
        "setGatewayFallbackEnabled(true)"));
    ASSERT_TRUE(catcher.GetNextResult()) << message_;
    EXPECT_EQ(GetPrefs()->GetBoolean(kIPFSAutoFallbackToGateway), true);
  }
}

}  // namespace
}  // namespace extensions
