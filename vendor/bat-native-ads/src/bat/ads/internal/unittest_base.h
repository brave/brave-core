/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_BASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_BASE_H_

#include <memory>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "bat/ads/database.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/browser_manager/browser_manager.h"
#include "bat/ads/internal/database/database_initialize.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "bat/ads/internal/tab_manager/tab_manager.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ads {

class UnitTestBase : public testing::Test {
 public:
  UnitTestBase();

  ~UnitTestBase() override;

  UnitTestBase(const UnitTestBase&) = delete;
  UnitTestBase& operator=(const UnitTestBase&) = delete;

  bool CopyFileFromTestPathToTempDir(const std::string& source_filename,
                                     const std::string& dest_filename) const;

  // If |integration_test| is set to true test the functionality and performance
  // under product-like circumstances with data to replicate live settings to
  // simulate what a real user scenario looks like from start to finish. You
  // must call |InitializeAds| manually after setting up your mocks
  void SetUpForTesting(const bool integration_test);

  void InitializeAds();

  AdsImpl* GetAds() const;
  AdRewards* GetAdRewards() const;

  // testing::Test implementation
  void SetUp() override;
  void TearDown() override;

 protected:
  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;

  // Fast-forwards virtual time by |time_delta|, causing all tasks on the main
  // thread and thread pool with a remaining delay less than or equal to
  // |time_delta| to be executed in their natural order before this returns. For
  // debugging purposes use |task_environment_.DescribePendingMainThreadTasks()|
  // to dump information about pending tasks
  void FastForwardClockBy(const base::TimeDelta& time_delta);

  // Fast-forwards virtual time to |time|, causing all tasks on the main thread
  // and thread pool with a remaining delay less than or equal to |time| to be
  // executed in their natural order before this returns. For debugging purposes
  // use |task_environment_.DescribePendingMainThreadTasks()| to dump
  // information about pending tasks
  void FastForwardClockTo(const base::Time& time);

  // Unlike |FastForwardClockBy|, |FastForwardClockTo| and |FastForwardBy|
  // AdvanceClock does not run tasks
  void AdvanceClockToMidnightUTC();
  void AdvanceClock(const base::Time& time);
  void AdvanceClock(const base::TimeDelta& time_delta);

  // Returns the delay until the next pending task of the main thread's
  // TaskRunner if there is one, otherwise it returns TimeDelta::Max()
  base::TimeDelta NextPendingTaskDelay() const;

  // Returns the number of pending tasks of the main thread's TaskRunner. When
  // debugging, you can use |task_environment_.DescribePendingMainThreadTasks()|
  // to see what those are
  size_t GetPendingTaskCount() const;

 private:
  bool setup_called_ = false;
  bool teardown_called_ = false;

  bool integration_test_ = false;

  std::unique_ptr<AdsClientHelper> ads_client_helper_;
  std::unique_ptr<Client> client_;
  std::unique_ptr<AdRewards> ad_rewards_;
  std::unique_ptr<AdNotifications> ad_notifications_;
  std::unique_ptr<ConfirmationsState> confirmations_state_;
  std::unique_ptr<database::Initialize> database_initialize_;
  std::unique_ptr<Database> database_;
  std::unique_ptr<BrowserManager> browser_manager_;
  std::unique_ptr<TabManager> tab_manager_;
  std::unique_ptr<UserActivity> user_activity_;
  std::unique_ptr<AdsImpl> ads_;

  void Initialize();
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_BASE_H_
