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
#include "base/memory/weak_ptr.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_mock.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_notifier_for_testing.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_mock.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

class Ads;
class Database;
class GlobalState;

namespace test {

class TestBase : public AdsClientNotifierForTesting, public ::testing::Test {
 public:
  TestBase();

  TestBase(const TestBase&) = delete;
  TestBase& operator=(const TestBase&) = delete;

  ~TestBase() override;

  // ::testing::Test:
  void SetUp() override;
  void TearDown() override;

 protected:
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

  // Returns the delay until the next pending task on the main thread's
  // TaskRunner if there is one, otherwise it returns `TimeDelta::Max`. See
  // `TaskEnvironment` for more detail.
  base::TimeDelta NextPendingTaskDelay() const;

  // Returns the number of pending tasks on the main thread's TaskRunner. When
  // debugging, you can use `task_environment_.DescribeCurrentTasks` to see
  // what those are. See `TaskEnvironment` for more detail.
  size_t GetPendingTaskCount() const;

  // Returns `true` if there are pending tasks on the main thread's TaskRunner.
  // When debugging, use `task_environment_.DescribeCurrentTasks` to see what
  // those are. See `TaskEnvironment` for more detail.
  bool HasPendingTasks() const;

  // Unlike `FastForwardClockToNextPendingTask`, `FastForwardClockTo`,
  // `FastForwardClockBy` and `SuspendedFastForwardClockBy`, `AdvanceClock*`
  // does not run tasks. See `TaskEnvironment` for more detail.
  void AdvanceClockBy(base::TimeDelta time_delta);
  void AdvanceClockTo(base::Time time);
  void AdvanceClockToLocalMidnight();
  void AdvanceClockToUTCMidnight();

  base::test::TaskEnvironment task_environment_;

  ::testing::NiceMock<PlatformHelperMock> platform_helper_mock_;

  ::testing::NiceMock<AdsClientMock> ads_client_mock_;

 private:
  void SimulateProfile();
  const base::FilePath& ProfilePath() const { return profile_dir_.GetPath(); }
  base::FilePath DatabasePath() const;

  void MockAdsClientNotifier();
  void MockAdsClient();
  void Mock();
  void MockDefaultAdsServiceState() const;

  void SetUpIntegrationTest();
  void SetUpIntegrationTestCallback(bool success);

  void SetUpUnitTest();

  base::ScopedTempDir profile_dir_;

  bool setup_called_ = false;
  bool teardown_called_ = false;

  brave_l10n::test::ScopedDefaultLocale scoped_default_locale_;

  ScopedBrowserVersionNumberForTesting scoped_browser_version_number_;

  std::unique_ptr<Database> database_;

  bool is_integration_test_ = false;

  // Integration tests only.
  std::unique_ptr<Ads> ads_;

  // Unit tests only.
  std::unique_ptr<GlobalState> global_state_;

  base::WeakPtrFactory<TestBase> weak_factory_{this};
};

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TEST_BASE_H_
