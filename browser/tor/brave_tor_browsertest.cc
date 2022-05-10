/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/process/process.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/tor/brave_tor_client_updater.h"
#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"
#include "brave/components/tor/tor_launcher_factory.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "brave/components/tor/tor_profile_service.h"
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

}  // namespace

class BraveTorTest : public InProcessBrowserTest {
 public:
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

  void CloseTorWindow(Profile* tor_profile) {
    TorProfileManager::CloseTorProfileWindows(tor_profile);
  }
};

IN_PROC_BROWSER_TEST_F(BraveTorTest, OpenCloseDisableTorWindow) {
  EXPECT_FALSE(TorProfileServiceFactory::IsTorDisabled());
  DownloadTorClient();

  MockTorLauncherObserver observer;
  Profile* tor_profile = nullptr;
  int tor_pid = 0;

  // Open Tor window, wait for the Tor process to start.
  {
    TorLauncherFactory::GetInstance()->AddObserver(&observer);

    base::RunLoop loop;
    EXPECT_CALL(observer, OnTorLaunched(testing::_, testing::_))
        .WillOnce(testing::Invoke(&loop, &base::RunLoop::Quit));

    tor_profile = OpenTorWindow();
    ASSERT_TRUE(tor_profile);

    loop.Run();

    tor_pid = TorLauncherFactory::GetInstance()->GetTorPid();
    EXPECT_TRUE(base::Process::Open(tor_pid).IsValid());

    TorLauncherFactory::GetInstance()->RemoveObserver(&observer);
  }

  // Close Tor window, expect the Tor process to die.
  {
    Browser* tor_browser = chrome::FindBrowserWithProfile(tor_profile);
    CloseTorWindow(tor_profile);
    ui_test_utils::BrowserChangeObserver(
        tor_browser, ui_test_utils::BrowserChangeObserver::ChangeType::kRemoved)
        .Wait();

    EXPECT_FALSE(base::Process::Open(tor_pid).IsValid());
  }

  // Disable tor, expect executables removed.
  {
    TorProfileServiceFactory::SetTorDisabled(true);
    EXPECT_TRUE(TorProfileServiceFactory::IsTorDisabled());

    content::RunAllTasksUntilIdle();

    EXPECT_FALSE(CheckComponentExists(tor::kTorClientComponentId));
  }
}
