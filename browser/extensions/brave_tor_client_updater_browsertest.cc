/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"

using extensions::ExtensionBrowserTest;

const char kTorClientUpdaterComponentTestId[] =
    "ngicbhhaldfdgmjhilmnleppfpmkgbbk";
const char kTorClientUpdaterComponentTestBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAncFKJWCX6RqCRYOR0t5a"
    "js7HWIVP3Ne272HZs3MqiaNvo9IikbPd4JnUMeQjLhkXTwxg6Up9Tmrgo3M8T91D"
    "dggzpAG4OlhKj3l3N5kZnj/CxQ73YVd41jHAF97lZVoD5VTCGtEelzA5eHI4N4Hd"
    "cvMiMvr/Kj9pdlJ+kbg5UZIXAYLXUB/NfBjKlpCTZ+Ys/2nxRN27kUVnrE/gTitE"
    "Aj1PZGOxJd1ZeiYc29j0ETf3AmOsZyVrIs6HJzHEJLnYQFaa76dRwVabm1Zt/28T"
    "+NJdHcu+jj2LIEcxmZ8TjtbK9kfWORHhA/ELjTx4ScvKfVKJgdLpxy5QOBFFnTLR"
    "QQIDAQAB";

class BraveTorClientUpdaterTest : public ExtensionBrowserTest {
 public:
  BraveTorClientUpdaterTest() {}

  void SetUp() override {
    InitEmbeddedTestServer();
    ExtensionBrowserTest::SetUp();
  }

  void PreRunTestOnMainThread() override {
    ExtensionBrowserTest::PreRunTestOnMainThread();
    WaitForTorClientUpdaterThread();
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
    extensions::BraveTorClientUpdater::SetComponentIdAndBase64PublicKeyForTest(
        component_id, component_base64_public_key);
  }

  bool InstallTorClientUpdater() {
#if defined(OS_WIN)
    return InstallTorClientUpdater("tor-client-updater-win");
#elif defined(OS_MAC)
    return InstallTorClientUpdater("tor-client-updater-mac");
#elif defined(OS_LINUX)
    return InstallTorClientUpdater("tor-client-updater-linux");
#else
    return false;
#endif
  }

  bool InstallTorClientUpdater(const std::string& extension_dir) {
    base::FilePath test_data_dir;
    GetTestDataDir(&test_data_dir);
    const extensions::Extension* tor_client_updater =
        InstallExtension(test_data_dir.AppendASCII("tor-client-updater")
                             .AppendASCII(extension_dir),
                         1);
    if (!tor_client_updater)
      return false;

    g_brave_browser_process->tor_client_updater()->OnComponentReady(
        tor_client_updater->id(), tor_client_updater->path(), "");
    WaitForTorClientUpdaterThread();

    return true;
  }

  void WaitForTorClientUpdaterThread() {
    scoped_refptr<base::ThreadTestHelper> io_helper(
        new base::ThreadTestHelper(
            g_brave_browser_process->tor_client_updater()->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }
};

// Load the Tor client updater extension and verify that it correctly
// installs the client.
IN_PROC_BROWSER_TEST_F(BraveTorClientUpdaterTest, TorClientInstalls) {
  SetComponentIdAndBase64PublicKeyForTest(
      kTorClientUpdaterComponentTestId,
      kTorClientUpdaterComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallTorClientUpdater());

  content::RunAllTasksUntilIdle();
  base::FilePath executable_path =
      g_brave_browser_process->tor_client_updater()->GetExecutablePath();
  ASSERT_TRUE(PathExists(executable_path));
}

// Load the Tor client updater extension and verify that we can launch
// the client.
IN_PROC_BROWSER_TEST_F(BraveTorClientUpdaterTest, TorClientLaunches) {
  SetComponentIdAndBase64PublicKeyForTest(
      kTorClientUpdaterComponentTestId,
      kTorClientUpdaterComponentTestBase64PublicKey);
  ASSERT_TRUE(InstallTorClientUpdater());

  content::RunAllTasksUntilIdle();
  base::FilePath executable_path =
      g_brave_browser_process->tor_client_updater()->GetExecutablePath();
  ASSERT_TRUE(PathExists(executable_path));

  base::CommandLine cmd_line(executable_path);
  base::Process tor_client_process =
      base::LaunchProcess(cmd_line, base::LaunchOptions());
  ASSERT_TRUE(tor_client_process.IsValid());
  ASSERT_TRUE(tor_client_process.Terminate(0, true));
}
