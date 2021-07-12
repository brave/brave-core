/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_updater.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"

using extensions::ExtensionBrowserTest;

static const char kWalletDataFilesUpdaterComponentTestId[] =
    "ngicbhhaldfdgmjhilmnleppfpmkgbbk";
static const char kWalletDataFilesUpdaterComponentTestBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAncFKJWCX6RqCRYOR0t5a"
    "js7HWIVP3Ne272HZs3MqiaNvo9IikbPd4JnUMeQjLhkXTwxg6Up9Tmrgo3M8T91D"
    "dggzpAG4OlhKj3l3N5kZnj/CxQ73YVd41jHAF97lZVoD5VTCGtEelzA5eHI4N4Hd"
    "cvMiMvr/Kj9pdlJ+kbg5UZIXAYLXUB/NfBjKlpCTZ+Ys/2nxRN27kUVnrE/gTitE"
    "Aj1PZGOxJd1ZeiYc29j0ETf3AmOsZyVrIs6HJzHEJLnYQFaa76dRwVabm1Zt/28T"
    "+NJdHcu+jj2LIEcxmZ8TjtbK9kfWORHhA/ELjTx4ScvKfVKJgdLpxy5QOBFFnTLR"
    "QQIDAQAB";

class WalletDataFilesUpdaterTest : public ExtensionBrowserTest {
 public:
  WalletDataFilesUpdaterTest() {}

  void SetUp() override {
    InitEmbeddedTestServer();
    ExtensionBrowserTest::SetUp();
  }

  void InitEmbeddedTestServer() {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void GetTestDataDir(base::FilePath* test_data_dir) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir);
  }

  bool PathExists(const base::FilePath& file_path) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::PathExists(file_path);
  }

  void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key) {
    brave_wallet::WalletDataFilesUpdater::
        SetComponentIdAndBase64PublicKeyForTest(component_id,
                                                component_base64_public_key);
  }

  bool InstallWalletDataFilesUpdater() {
    base::FilePath test_data_dir;
    GetTestDataDir(&test_data_dir);
    const extensions::Extension* wallet_data_files_updater =
        InstallExtension(test_data_dir.AppendASCII("ipfs-client-updater")
                             .AppendASCII("ipfs-client-updater-win"),
                         1);
    if (!wallet_data_files_updater)
      return false;

    g_brave_browser_process->wallet_data_files_updater()->OnComponentReady(
        wallet_data_files_updater->id(), wallet_data_files_updater->path(), "");
    WaitForWalletDataFilesUpdaterThread();
    WaitForMainThreadTasksToFinish();
    return true;
  }

  void WaitForWalletDataFilesUpdaterThread() {
    scoped_refptr<base::ThreadTestHelper> io_helper(new base::ThreadTestHelper(
        g_brave_browser_process->wallet_data_files_updater()->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }

  void WaitForMainThreadTasksToFinish() {
    base::RunLoop loop;
    loop.RunUntilIdle();
  }

  void SetDataFilesPath(const base::FilePath& path) {
    g_brave_browser_process->wallet_data_files_updater()->SetPath(path);
  }
};

// Load the Wallet data files updater extension and verify that it correctly
// installs the client.
IN_PROC_BROWSER_TEST_F(WalletDataFilesUpdaterTest,
                       WalletDataFilesUpdaterInstalls) {
  SetComponentIdAndBase64PublicKeyForTest(
      kWalletDataFilesUpdaterComponentTestId,
      kWalletDataFilesUpdaterComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallWalletDataFilesUpdater());
  base::FilePath executable_path =
      g_brave_browser_process->wallet_data_files_updater()->GetExecutablePath();
  ASSERT_TRUE(PathExists(executable_path));
}

IN_PROC_BROWSER_TEST_F(WalletDataFilesUpdaterTest, WalletDataFilesReady) {
  brave_wallet::BraveWalletService* wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetInstance()->GetForContext(
          profile());
  ASSERT_TRUE(wallet_service);
  ASSERT_FALSE(wallet_service->IsIPFSExecutableAvailable());
  ASSERT_TRUE(wallet_service->GetPath().empty());
  SetComponentIdAndBase64PublicKeyForTest(
      kWalletDataFilesUpdaterComponentTestId,
      kWalletDataFilesUpdaterComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallWalletDataFilesUpdater());
  base::FilePath executable_path =
      g_brave_browser_process->wallet_data_files_updater()->GetExecutablePath();
  ASSERT_TRUE(PathExists(executable_path));

  EXPECT_EQ(wallet_service->GetPath(), executable_path);
  ASSERT_TRUE(wallet_service->IsIPFSExecutableAvailable());

  base::FilePath new_path(FILE_PATH_LITERAL("newpath"));
  SetDataFilesPath(new_path);
  EXPECT_EQ(wallet_service->GetPath(), new_path);
  ASSERT_TRUE(wallet_service->IsIPFSExecutableAvailable());
}
