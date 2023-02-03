/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_BASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_BASE_H_

#include <memory>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/browser/browser_manager.h"
#include "bat/ads/internal/common/platform/platform_helper_mock.h"
#include "bat/ads/internal/covariates/covariate_manager.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_manager.h"
#include "bat/ads/internal/database/database_manager.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/diagnostics/diagnostic_manager.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/internal/locale/locale_manager.h"
#include "bat/ads/internal/prefs/pref_manager.h"
#include "bat/ads/internal/resources/resource_manager.h"
#include "bat/ads/internal/tabs/tab_manager.h"
#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_manager.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_manager.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_l10n::test {
class ScopedDefaultLocale;
}  // namespace brave_l10n::test

namespace ads {

class Database;

class UnitTestBase : public testing::Test {
 public:
  UnitTestBase();

  UnitTestBase(const UnitTestBase& other) = delete;
  UnitTestBase& operator=(const UnitTestBase& other) = delete;

  UnitTestBase(UnitTestBase&& other) noexcept = delete;
  UnitTestBase& operator=(UnitTestBase&& other) noexcept = delete;

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
  AdsImpl* GetAds() const;

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
  // |FastForwardClockBy| AdvanceClock does not run tasks. See |TaskEnvironment|
  // for more detail.
  void AdvanceClockBy(base::TimeDelta time_delta);
  void AdvanceClockTo(base::Time time);
  void AdvanceClockToMidnight(bool is_local);

  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;

 private:
  void Initialize();

  void SetDefaultMocks();

  void SetDefaultPrefs();

  void SetUpIntegrationTest();

  base::ScopedTempDir temp_dir_;

  bool is_integration_test_ = false;

  bool setup_called_ = false;
  bool teardown_called_ = false;

  std::unique_ptr<brave_l10n::test::ScopedDefaultLocale> scoped_default_locale_;

  std::unique_ptr<AdsImpl> ads_;

  std::unique_ptr<Database> database_;

  std::unique_ptr<AdsClientHelper> ads_client_helper_;

  std::unique_ptr<BrowserManager> browser_manager_;
  std::unique_ptr<ClientStateManager> client_state_manager_;
  std::unique_ptr<ConfirmationStateManager> confirmation_state_manager_;
  std::unique_ptr<CovariateManager> covariate_manager_;
  std::unique_ptr<DatabaseManager> database_manager_;
  std::unique_ptr<DiagnosticManager> diagnostic_manager_;
  std::unique_ptr<FlagManager> flag_manager_;
  std::unique_ptr<HistoryManager> history_manager_;
  std::unique_ptr<IdleDetectionManager> idle_detection_manager_;
  std::unique_ptr<LocaleManager> locale_manager_;
  std::unique_ptr<NotificationAdManager> notification_ad_manager_;
  std::unique_ptr<PrefManager> pref_manager_;
  std::unique_ptr<ResourceManager> resource_manager_;
  std::unique_ptr<TabManager> tab_manager_;
  std::unique_ptr<UserActivityManager> user_activity_manager_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_BASE_H_
