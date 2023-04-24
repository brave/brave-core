/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_BASE_H_

#include <memory>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_ads/core/ads_client_notifier.h"
#include "brave/components/brave_ads/core/internal/ads_client_mock.h"
#include "brave/components/brave_ads/core/internal/ads_impl.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_mock.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

using ::testing::NiceMock;

class Database;
class GlobalState;

class UnitTestBase : public AdsClientNotifier, public testing::Test {
 public:
  UnitTestBase();

  UnitTestBase(const UnitTestBase&) = delete;
  UnitTestBase& operator=(const UnitTestBase&) = delete;

  UnitTestBase(UnitTestBase&&) noexcept = delete;
  UnitTestBase& operator=(UnitTestBase&&) noexcept = delete;

  ~UnitTestBase() override;

  // testing::Test:
  void SetUp() override;
  void TearDown() override;

 protected:
  // Override |SetUp| and call |SetUpForTesting| with |is_integration_test| set
  // to |true| to test functionality and performance under product-like
  // circumstances with data to replicate live settings to simulate what a real
  // user scenario looks like from start to finish. You should mock AdsClient
  // and copy mock files and directories before initialization in |SetUpMocks|.
  void SetUpForTesting(bool is_integration_test);

  // Override |SetUpMocks| to mock AdsClient and to copy mock files and
  // directories before initialization.
  virtual void SetUpMocks() {}

  // Convenience function for accessing AdsImpl from integration tests.
  AdsImpl& GetAds() const;

  // Copies a single file from "data/test" to the temp path. Use
  // |CopyDirectoryFromTestPathToTempPath| to copy directories.
  bool CopyFileFromTestPathToTempPath(const std::string& from_path,
                                      const std::string& to_path) const;
  bool CopyFileFromTestPathToTempPath(const std::string& path) const;

  // Copies the given path from "data/test", and all subdirectories and their
  // contents as well to the temp directory.
  bool CopyDirectoryFromTestPathToTempPath(const std::string& from_path,
                                           const std::string& to_path) const;
  bool CopyDirectoryFromTestPathToTempPath(const std::string& path) const;

  // Fast-forwards virtual time by |time_delta|, causing all tasks on the main
  // thread and thread pool with a remaining delay less than or equal to
  // |time_delta| to be executed in their natural order before this returns. For
  // debugging purposes use |task_environment_.DescribeCurrentTasks()| to dump
  // information about pending tasks. See |TaskEnvironment| for more detail.
  void FastForwardClockBy(base::TimeDelta time_delta);

  // Fast-forwards virtual time to |time|, causing all tasks on the main thread
  // and thread pool with a remaining delay less than or equal to |time| to be
  // executed in their natural order before this returns. For debugging purposes
  // use |task_environment_.DescribeCurrentTasks()| to dump information about
  // pending tasks. See |TaskEnvironment| for more detail.
  void FastForwardClockTo(base::Time time);

  // Fast-forwards virtual time to the next pending task, causing the task on
  // the main thread and thread pool to be executed before this returns. For
  // debugging purposes use |task_environment_.DescribeCurrentTasks()| to dump
  // information about pending tasks. See |TaskEnvironment| for more detail.
  void FastForwardClockToNextPendingTask();

  // Returns the delay until the next pending task on the main thread's
  // TaskRunner if there is one, otherwise it returns TimeDelta::Max(). See
  // |TaskEnvironment| for more detail.
  base::TimeDelta NextPendingTaskDelay() const;

  // Returns the number of pending tasks on the main thread's TaskRunner. When
  // debugging, you can use |task_environment_.DescribeCurrentTasks()| to see
  // what those are. See |TaskEnvironment| for more detail.
  size_t GetPendingTaskCount() const;

  // Returns |true| if there are pending tasks on the main thread's TaskRunner.
  // When debugging, use |task_environment_.DescribeCurrentTasks()| to see what
  // those are. See |TaskEnvironment| for more detail.
  bool HasPendingTasks() const;

  // Unlike |FastForwardClockToNextPendingTask|, |FastForwardClockTo| and
  // |FastForwardClockBy|, AdvanceClock does not run tasks. See
  // |TaskEnvironment| for more detail.
  void AdvanceClockBy(base::TimeDelta time_delta);
  void AdvanceClockTo(base::Time time);
  void AdvanceClockToMidnight(bool is_local);

  base::test::TaskEnvironment task_environment_;

  NiceMock<AdsClientMock> ads_client_mock_;
  NiceMock<PlatformHelperMock> platform_helper_mock_;

 private:
  void Initialize();

  void MockAdsClientAddObserver();

  void MockDefaultAdsClient();

  void MockSetBooleanPref(AdsClientMock& mock);
  void MockSetIntegerPref(AdsClientMock& mock);
  void MockSetDoublePref(AdsClientMock& mock);
  void MockSetStringPref(AdsClientMock& mock);
  void MockSetInt64Pref(AdsClientMock& mock);
  void MockSetUint64Pref(AdsClientMock& mock);
  void MockSetTimePref(AdsClientMock& mock);
  void MockSetDictPref(AdsClientMock& mock);
  void MockSetListPref(AdsClientMock& mock);

  void MockDefaultPrefs();

  void SetUpIntegrationTest();

  base::ScopedTempDir temp_dir_;

  bool is_integration_test_ = false;

  bool setup_called_ = false;
  bool teardown_called_ = false;

  brave_l10n::test::ScopedDefaultLocale scoped_default_locale_;

  std::unique_ptr<AdsImpl> ads_;

  std::unique_ptr<Database> database_;

  std::unique_ptr<GlobalState> global_state_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_BASE_H_
