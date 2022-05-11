/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/process/process.h"
#include "base/process/process_iterator.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/thread_test_helper.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/tor/brave_tor_client_updater.h"
#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "brave/components/tor/tor_profile_service.h"
#include "brave/components/tor/tor_utils.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "gmock/gmock.h"

namespace {

struct MockTorLauncherObserver : public TorLauncherObserver {
 public:
  MOCK_METHOD(void, OnTorLauncherCrashed, (), (override));
  MOCK_METHOD(void, OnTorCrashed, (int64_t pid), (override));
  MOCK_METHOD(void, OnTorLaunched, (bool result, int64_t pid), (override));
  MOCK_METHOD(void, OnTorControlReady, (), (override));
  MOCK_METHOD(void, OnTorNewProxyURI, (const std::string& uri), (override));
  MOCK_METHOD(void, OnTorCircuitEstablished, (bool result), (override));
  MOCK_METHOD(void,
              OnTorInitializing,
              (const std::string& percentage),
              (override));
  MOCK_METHOD(void, OnTorControlEvent, (const std::string& event), (override));
  MOCK_METHOD(void, OnTorLogUpdated, (), (override));
};

void DownloadTorComponent(const std::string& component_id) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  base::FilePath test_data_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

#if BUILDFLAG(IS_WIN)
  const char platform[] = "win";
#elif BUILDFLAG(IS_MAC)
  const char platform[] = "mac";
#elif BUILDFLAG(IS_LINUX)
  const char platform[] = "linux";
#endif

  const auto component_dir = test_data_dir.AppendASCII("tor")
                                 .AppendASCII("components")
                                 .AppendASCII(platform)
                                 .AppendASCII(component_id);
  ASSERT_TRUE(base::PathExists(component_dir)) << component_dir;

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);

  ASSERT_TRUE(base::CopyDirectory(
      component_dir, user_data_dir.AppendASCII(component_id), true));
}

bool CheckComponentExists(const std::string& component_id) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  return base::PathExists(user_data_dir.AppendASCII(component_id));
}

void NonBlockingDelay(const base::TimeDelta& delay) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
  run_loop.Run();
}

}  // namespace

class BraveTorTest : public InProcessBrowserTest {
 public:
  struct TorInfo {
    Profile* tor_profile = nullptr;
    int tor_pid = 0;
  };

  BraveTorTest() = default;
  ~BraveTorTest() override = default;

  void SetUp() override {
    brave::RegisterPathProvider();
    InProcessBrowserTest::SetUp();
  }

  void DownloadTorClient() const {
    DownloadTorComponent(tor::kTorClientComponentId);
  }

  void DownloadTorPluggableTransports() const {
    DownloadTorComponent(tor::kTorPluggableTransportComponentId);
  }

  Profile* OpenTorWindow() {
    base::RunLoop loop;
    Profile* tor_profile = nullptr;
    TorProfileManager::SwitchToTorProfile(
        browser()->profile(), base::BindLambdaForTesting([&](Profile* p) {
          tor_profile = p;
          loop.Quit();
        }));
    loop.Run();
    return tor_profile;
  }

  TorInfo WaitForTorLaunched() {
    MockTorLauncherObserver observer;
    TorLauncherFactory::GetInstance()->AddObserver(&observer);

    base::RunLoop loop;
    EXPECT_CALL(observer, OnTorLaunched(testing::_, testing::_))
        .WillOnce(testing::Invoke(&loop, &base::RunLoop::Quit));

    Profile* tor_profile = OpenTorWindow();

    loop.Run();

    int tor_pid = TorLauncherFactory::GetInstance()->GetTorPid();

    TorLauncherFactory::GetInstance()->RemoveObserver(&observer);

    return {tor_profile, tor_pid};
  }

  void CloseTorWindow(Profile* tor_profile) {
    TorProfileManager::CloseTorProfileWindows(tor_profile);
  }

  void WaitForUpdaterThread(brave_component_updater::BraveComponent* updater) {
    scoped_refptr<base::ThreadTestHelper> io_helper(
        new base::ThreadTestHelper(updater->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }

  void WaitForProcess(const std::string& name) {
    while (!base::GetProcessCount(base::FilePath::FromASCII(name).value(),
                                  nullptr)) {
      NonBlockingDelay(base::Milliseconds(25));
    }
  }

  void WaitProcessExit(const std::string& name) {
    while (base::GetProcessCount(base::FilePath::FromASCII(name).value(),
                                 nullptr)) {
      NonBlockingDelay(base::Milliseconds(25));
    }
  }
};

IN_PROC_BROWSER_TEST_F(BraveTorTest, OpenCloseDisableTorWindow) {
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled());
  DownloadTorClient();

  // Open Tor window, wait for the Tor process to start.
  auto tor = WaitForTorLaunched();
  EXPECT_TRUE(base::Process::Open(tor.tor_pid).IsValid());
  ASSERT_TRUE(tor.tor_profile);

  // Close Tor window, expect the Tor process to die.
  {
    Browser* tor_browser = chrome::FindBrowserWithProfile(tor.tor_profile);
    CloseTorWindow(tor.tor_profile);
    ui_test_utils::BrowserChangeObserver(
        tor_browser, ui_test_utils::BrowserChangeObserver::ChangeType::kRemoved)
        .Wait();

    EXPECT_FALSE(base::Process::Open(tor.tor_pid).IsValid());
  }

  // Disable tor, expect executables are removed.
  {
    TorProfileServiceFactory::SetTorDisabled(true);
    EXPECT_TRUE(TorProfileServiceFactory::IsTorDisabled());

    WaitForUpdaterThread(g_brave_browser_process->tor_client_updater());
    content::RunAllTasksUntilIdle();

    EXPECT_FALSE(CheckComponentExists(tor::kTorClientComponentId));
  }
}

IN_PROC_BROWSER_TEST_F(BraveTorTest, PRE_SetupBridges) {
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled());
  DownloadTorClient();

  // No bridges by default.
  auto bridges_config = TorProfileServiceFactory::GetTorBridgesConfig();
  EXPECT_FALSE(bridges_config.use_bridges);
  EXPECT_EQ(tor::BridgesConfig::BuiltinType::kNone, bridges_config.use_builtin);
  EXPECT_TRUE(bridges_config.bridges.empty());

  // Open Tor window, wait for the Tor process to start.
  auto tor = WaitForTorLaunched();
  EXPECT_TRUE(tor.tor_profile);

  // Pluggable transport component isn't installed.
  EXPECT_FALSE(CheckComponentExists(tor::kTorPluggableTransportComponentId));
  EXPECT_EQ(
      0, base::GetProcessCount(
             base::FilePath::FromASCII(tor::kSnowflakeExecutableName).value(),
             nullptr));
  EXPECT_EQ(0, base::GetProcessCount(
                   base::FilePath::FromASCII(tor::kObfs4ExecutableName).value(),
                   nullptr));

  // Enable bridges
  DownloadTorPluggableTransports();

  bridges_config.use_bridges = true;
  bridges_config.bridges.push_back(
      "snowflake 192.0.2.3:1 2B280B23E1107BB62ABFC40DDCC8824814F80A72");
  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);

  // Wait Snowflake executable is launched.
  EXPECT_TRUE(CheckComponentExists(tor::kTorPluggableTransportComponentId));
  WaitForProcess(tor::kSnowflakeExecutableName);

  EXPECT_TRUE(
      g_brave_browser_process->tor_pluggable_transport_updater()->IsReady());

  // Add obfs config.
  bridges_config.bridges.push_back(
      "obfs4 144.217.20.138:80 FB70B257C162BF1038CA669D568D76F5B7F0BABB "
      "cert=vYIV5MgrghGQvZPIi1tJwnzorMgqgmlKaB77Y3Z9Q/"
      "v94wZBOAXkW+fdx4aSxLVnKO+xNw iat-mode=0");
  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);

  // Obfs4 executable is launched.
  WaitForProcess(tor::kSnowflakeExecutableName);
  WaitForProcess(tor::kObfs4ExecutableName);

  // Disable tor.
  TorProfileServiceFactory::SetTorDisabled(true);
  EXPECT_TRUE(TorProfileServiceFactory::IsTorDisabled());
  WaitForUpdaterThread(g_brave_browser_process->tor_client_updater());
  WaitForUpdaterThread(
      g_brave_browser_process->tor_pluggable_transport_updater());
}

IN_PROC_BROWSER_TEST_F(BraveTorTest, SetupBridges) {
  // Tor is disabled in PRE, check pluggable transport are removed.
  EXPECT_FALSE(CheckComponentExists(tor::kTorPluggableTransportComponentId));

  // Pluggable transport processes was terminated at exit.
  EXPECT_EQ(
      0, base::GetProcessCount(
             base::FilePath::FromASCII(tor::kSnowflakeExecutableName).value(),
             nullptr));
  EXPECT_EQ(0, base::GetProcessCount(
                   base::FilePath::FromASCII(tor::kObfs4ExecutableName).value(),
                   nullptr));
}

IN_PROC_BROWSER_TEST_F(BraveTorTest, ResetBridges) {
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled());
  DownloadTorClient();
  DownloadTorPluggableTransports();

  auto bridges_config = TorProfileServiceFactory::GetTorBridgesConfig();
  bridges_config.use_bridges = true;
  bridges_config.bridges.push_back(
      "snowflake 192.0.2.3:1 2B280B23E1107BB62ABFC40DDCC8824814F80A72");
  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);

  // Open Tor window, wait for the Tor process to start.
  auto tor = WaitForTorLaunched();
  EXPECT_TRUE(tor.tor_profile);

  // Wait Snowflake executable is launched.
  EXPECT_TRUE(CheckComponentExists(tor::kTorPluggableTransportComponentId));
  WaitForProcess(tor::kSnowflakeExecutableName);

  // Reset bridges
  bridges_config.use_bridges = false;
  TorProfileServiceFactory::SetTorBridgesConfig(bridges_config);
  WaitProcessExit(tor::kSnowflakeExecutableName);
}
