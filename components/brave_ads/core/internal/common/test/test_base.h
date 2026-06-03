/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TEST_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TEST_BASE_H_

#include <cstddef>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/test/scoped_command_line.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_ads/core/internal/ads_client/test/ads_client_mock.h"
#include "brave/components/brave_ads/core/internal/application_state/test/fake_browser_version.h"
#include "brave/components/brave_ads/core/internal/common/locale/test/fake_locale.h"
#include "brave/components/brave_ads/core/internal/common/operating_system/test/fake_operating_system.h"
#include "brave/components/brave_ads/core/internal/common/test/tab_test_helper.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

class Ads;
class GlobalState;

namespace test {

class TestBase : public ::testing::Test {
 public:
  TestBase(const TestBase&) = delete;
  TestBase& operator=(const TestBase&) = delete;

  ~TestBase() override;

  // ::testing::Test:
  void SetUp() override;
  void TearDown() override;

 protected:
  TestBase();

  // Pass `is_integration_test` as `true` to test functionality and performance
  // under product-like circumstances with data to replicate live settings to
  // simulate what a real user scenario looks like from start to finish.
  explicit TestBase(bool is_integration_test);

  // Override `SetUp` and call `test::TestBase::SetUp` with
  // `is_integration_test` set to `true` to test functionality and performance
  // under product-like circumstances with data to replicate live settings to
  // simulate what a real user scenario looks like from start to finish.
  void SetUp(bool is_integration_test);

  // Override `SetUpMocks` to mock command-line switches, the file system,
  // preferences, and `AdsClient` before initialization.
  virtual void SetUpMocks() {}

  // Convenience function for accessing Ads from integration tests.
  Ads& GetAds() const;

  // Copies a single file from "test/data" to the temp profile path. Use
  // `CopyDirectoryFromTestDataPathToProfilePath` to copy directories.
  [[nodiscard]] bool CopyFileFromTestDataPathToProfilePath(
      const std::string& from_path,
      const std::string& to_path) const;
  [[nodiscard]] bool CopyFileFromTestDataPathToProfilePath(
      const std::string& path) const;

  // Copies the given path from "test/data" and its contents to the temporary
  // directory. If recursive` is `true`, it also copies all subdirectories and
  // their contents.
  [[nodiscard]] bool CopyDirectoryFromTestDataPathToProfilePath(
      const std::string& from_path,
      const std::string& to_path,
      bool recursive) const;
  [[nodiscard]] bool CopyDirectoryFromTestDataPathToProfilePath(
      const std::string& path,
      bool recursive) const;

  // Fast-forwards virtual time by `time_delta`, causing all tasks on the main
  // thread and thread pool with a remaining delay less than or equal to
  // `time_delta` to be executed in their natural order before this returns. For
  // debugging purposes use `task_environment_.DescribeCurrentTasks` to dump
  // information about pending tasks. See `TaskEnvironment` for more detail.
  void FastForwardClockBy(base::TimeDelta time_delta);

  // Similar to `FastForwardClockBy` but doesn't advance `base::LiveTicks`,
  // behaving as if the system was suspended for `time_delta` and immediately
  // woken up. See `TaskEnvironment` for more detail.
  void SuspendedFastForwardClockBy(base::TimeDelta time_delta);

  // Fast-forwards virtual time to `time`, causing all tasks on the main thread
  // and thread pool with a remaining delay less than or equal to `time` to be
  // executed in their natural order before this returns. For debugging purposes
  // use `task_environment_.DescribeCurrentTasks` to dump information about
  // pending tasks. See `TaskEnvironment` for more detail.
  void FastForwardClockTo(base::Time time);

  // Fast-forwards virtual time to the next pending task, causing the task on
  // the main thread and thread pool to be executed before this returns. For
  // debugging purposes use `task_environment_.DescribeCurrentTasks` to dump
  // information about pending tasks. See `TaskEnvironment` for more detail.
  void FastForwardClockToNextPendingTask();

  // Flushes any immediately-due tasks then returns the delay until the next
  // pending task on the main thread's TaskRunner, or `TimeDelta::Max` if there
  // are none. See `TaskEnvironment` for more detail.
  base::TimeDelta NextPendingTaskDelay();

  // Flushes any immediately-due tasks then returns the number of pending tasks
  // on the main thread's TaskRunner. When debugging, you can use
  // `task_environment_.DescribeCurrentTasks` to see what those are. See
  // `TaskEnvironment` for more detail.
  size_t GetPendingTaskCount();

  // Flushes any immediately-due tasks then returns `true` if there are pending
  // tasks on the main thread's TaskRunner. When debugging, use
  // `task_environment_.DescribeCurrentTasks` to see what those are. See
  // `TaskEnvironment` for more detail.
  bool HasPendingTasks();

  // Unlike `FastForwardClockToNextPendingTask`, `FastForwardClockTo`,
  // `FastForwardClockBy` and `SuspendedFastForwardClockBy`, `AdvanceClock*`
  // does not run tasks. See `TaskEnvironment` for more detail.
  void AdvanceClockBy(base::TimeDelta time_delta);
  void AdvanceClockTo(base::Time time);
  void AdvanceClockToLocalMidnight();
  void AdvanceClockToUTCMidnight();

  base::test::TaskEnvironment task_environment_;

  FakeBrowserVersion fake_browser_version_;
  FakeLocale fake_locale_;
  FakeOperatingSystem fake_operating_system_;

  ::testing::NiceMock<AdsClientMock> ads_client_mock_;
  AdsClientNotifier ads_client_notifier_;

  TabHelper tab_helper_;

 private:
  void SimulateProfile();
  const base::FilePath& ProfilePath() const { return profile_dir_.GetPath(); }
  base::FilePath DatabasePath() const;

  void MockAdsClientNotifier();
  void MockAdsClient();
  void SetUpEnvironment();
  void SetUpDefaultAdsServiceState(
      base::OnceClosure initialized_callback) const;

  void SetUpIntegrationTest();
  void SetUpIntegrationTestCallback(bool success);

  void SetUpUnitTest();

  void FlushImmediateTasks();

  // Captures the original command line and restores it on destruction to
  // prevent `AppendCommandLineSwitches` in `SetUpMocks` from leaking between
  // tests.
  base::test::ScopedCommandLine scoped_command_line_;

  base::ScopedTempDir profile_dir_;

  bool setup_called_ = false;
  bool teardown_called_ = false;

  bool is_integration_test_ = false;  // Defaults to unit test.

  // Integration tests only.
  std::unique_ptr<Ads> ads_;

  // Unit tests only.
  std::unique_ptr<GlobalState> global_state_;

  base::WeakPtrFactory<TestBase> weak_factory_{this};
};

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TEST_BASE_H_
