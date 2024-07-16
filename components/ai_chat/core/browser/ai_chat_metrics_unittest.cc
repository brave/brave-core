/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"

#include <limits>
#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AIChatMetricsUnitTest : public testing::Test {
 public:
  AIChatMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    auto* registry = local_state_.registry();
    AIChatMetrics::RegisterPrefs(registry);
    task_environment_.FastForwardBy(base::Days(30));
    ai_chat_metrics_ = std::make_unique<AIChatMetrics>(&local_state_);
  }

  void RecordPrompts(bool new_chats, size_t chat_count) {
    for (size_t i = 0; i < chat_count; i++) {
      if (new_chats) {
        ai_chat_metrics_->RecordNewChat();
      }
      ai_chat_metrics_->RecordNewPrompt();
    }
    task_environment_.FastForwardBy(base::Seconds(5));
  }

  AIChatMetrics::RetrievePremiumStatusCallback GetPremiumCallback() {
    bool is_premium = is_premium_;
    return base::BindLambdaForTesting(
        [is_premium](mojom::Service::GetPremiumStatusCallback callback) {
          std::move(callback).Run(is_premium ? mojom::PremiumStatus::Active
                                             : mojom::PremiumStatus::Inactive,
                                  nullptr);
        });
  }

 protected:
  bool is_premium_ = false;
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<AIChatMetrics> ai_chat_metrics_;
};

TEST_F(AIChatMetricsUnitTest, Enabled) {
  ai_chat_metrics_->RecordEnabled(false, false, GetPremiumCallback());
  histogram_tester_.ExpectTotalCount(kEnabledHistogramName, 0);
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());
  histogram_tester_.ExpectUniqueSample(kEnabledHistogramName, 1, 1);
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());
  histogram_tester_.ExpectUniqueSample(kEnabledHistogramName, 1, 2);

  is_premium_ = true;
  ai_chat_metrics_->OnPremiumStatusUpdated(false, mojom::PremiumStatus::Active,
                                           nullptr);
  histogram_tester_.ExpectBucketCount(kEnabledHistogramName, 2, 1);

  is_premium_ = false;
  // should check premium status only after 24 hours have elapsed
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());
  histogram_tester_.ExpectBucketCount(kEnabledHistogramName, 2, 2);

  task_environment_.FastForwardBy(base::Days(1));
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());
  histogram_tester_.ExpectBucketCount(kEnabledHistogramName, 1, 3);

  histogram_tester_.ExpectTotalCount(kEnabledHistogramName, 5);
}

TEST_F(AIChatMetricsUnitTest, ChatCount) {
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());
  histogram_tester_.ExpectTotalCount(kChatCountHistogramName, 0);

  RecordPrompts(true, 1);
  histogram_tester_.ExpectUniqueSample(kChatCountHistogramName, 0, 1);

  RecordPrompts(true, 3);
  histogram_tester_.ExpectBucketCount(kChatCountHistogramName, 1, 1);

  RecordPrompts(true, 3);
  histogram_tester_.ExpectBucketCount(kChatCountHistogramName, 1, 1);
  histogram_tester_.ExpectBucketCount(kChatCountHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(kChatCountHistogramName, 3);

  task_environment_.FastForwardBy(base::Days(4));

  histogram_tester_.ExpectBucketCount(kChatCountHistogramName, 2, 5);
  histogram_tester_.ExpectTotalCount(kChatCountHistogramName, 7);

  task_environment_.FastForwardBy(base::Days(3));

  histogram_tester_.ExpectBucketCount(kChatCountHistogramName, 2, 7);
  histogram_tester_.ExpectTotalCount(kChatCountHistogramName, 9);

  task_environment_.FastForwardBy(base::Days(3));
  histogram_tester_.ExpectTotalCount(kChatCountHistogramName, 9);
}

TEST_F(AIChatMetricsUnitTest, AvgPromptsPerChat) {
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());
  histogram_tester_.ExpectTotalCount(kAvgPromptCountHistogramName, 0);

  RecordPrompts(true, 1);
  histogram_tester_.ExpectUniqueSample(kAvgPromptCountHistogramName, 0, 1);

  RecordPrompts(false, 2);
  histogram_tester_.ExpectBucketCount(kAvgPromptCountHistogramName, 1, 1);

  RecordPrompts(false, 4);
  histogram_tester_.ExpectBucketCount(kAvgPromptCountHistogramName, 2, 1);

  RecordPrompts(true, 1);
  histogram_tester_.ExpectBucketCount(kAvgPromptCountHistogramName, 1, 2);
  histogram_tester_.ExpectTotalCount(kAvgPromptCountHistogramName, 4);

  task_environment_.FastForwardBy(base::Days(4));

  histogram_tester_.ExpectBucketCount(kAvgPromptCountHistogramName, 1, 6);
  histogram_tester_.ExpectTotalCount(kAvgPromptCountHistogramName, 8);

  task_environment_.FastForwardBy(base::Days(3));

  histogram_tester_.ExpectBucketCount(kAvgPromptCountHistogramName, 1, 8);
  histogram_tester_.ExpectTotalCount(kAvgPromptCountHistogramName, 10);

  task_environment_.FastForwardBy(base::Days(3));
  histogram_tester_.ExpectTotalCount(kAvgPromptCountHistogramName, 10);
}

TEST_F(AIChatMetricsUnitTest, UsageDailyWeeklyAndMonthly) {
  is_premium_ = false;

  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());
  task_environment_.RunUntilIdle();
  histogram_tester_.ExpectTotalCount(kUsageDailyHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kUsageWeeklyHistogramName, 0);
  histogram_tester_.ExpectTotalCount(kUsageMonthlyHistogramName, 0);

  RecordPrompts(true, 1);
  histogram_tester_.ExpectUniqueSample(kUsageDailyHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kUsageWeeklyHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kUsageMonthlyHistogramName, 1, 1);

  is_premium_ = true;
  ai_chat_metrics_->OnPremiumStatusUpdated(false, mojom::PremiumStatus::Active,
                                           nullptr);
  RecordPrompts(true, 1);
  histogram_tester_.ExpectBucketCount(kUsageDailyHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kUsageWeeklyHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kUsageMonthlyHistogramName, 2, 1);

  histogram_tester_.ExpectTotalCount(kUsageDailyHistogramName, 2);
  histogram_tester_.ExpectTotalCount(kUsageWeeklyHistogramName, 2);
  histogram_tester_.ExpectTotalCount(kUsageMonthlyHistogramName, 2);
}

TEST_F(AIChatMetricsUnitTest, FeatureUsageNotNewUser) {
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());

  // Should not mark as new user, even if first/last timestamps were not
  // recorded internally
  histogram_tester_.ExpectUniqueSample(kNewUserReturningHistogramName, 1, 1);

  RecordPrompts(true, 1);
  histogram_tester_.ExpectUniqueSample(kNewUserReturningHistogramName, 1, 2);
}

TEST_F(AIChatMetricsUnitTest, FeatureUsage) {
  ai_chat_metrics_->RecordEnabled(false, false, GetPremiumCallback());
  histogram_tester_.ExpectUniqueSample(kNewUserReturningHistogramName, 0, 1);

  ai_chat_metrics_->RecordEnabled(true, true, GetPremiumCallback());
  histogram_tester_.ExpectUniqueSample(kNewUserReturningHistogramName, 0, 2);
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 0);

  RecordPrompts(true, 1);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 1);
  histogram_tester_.ExpectUniqueSample(kLastUsageTimeHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(14));

  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 1, 8);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 3, 1);
}

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
TEST_F(AIChatMetricsUnitTest, AcquisitionSource) {
  histogram_tester_.ExpectTotalCount(kAcquisitionSourceHistogramName, 0);

  ai_chat_metrics_->HandleOpenViaEntryPoint(EntryPoint::kSidebar);
  histogram_tester_.ExpectTotalCount(kAcquisitionSourceHistogramName, 0);

  ai_chat_metrics_->RecordEnabled(true, true, GetPremiumCallback());
  histogram_tester_.ExpectUniqueSample(kAcquisitionSourceHistogramName, 1, 1);

  ai_chat_metrics_->RecordOmniboxOpen();
  histogram_tester_.ExpectUniqueSample(kAcquisitionSourceHistogramName, 1, 1);

  ai_chat_metrics_->RecordEnabled(true, true, GetPremiumCallback());
  histogram_tester_.ExpectTotalCount(kAcquisitionSourceHistogramName, 2);
  histogram_tester_.ExpectBucketCount(kAcquisitionSourceHistogramName, 0, 1);
}

TEST_F(AIChatMetricsUnitTest, OmniboxOpens) {
  histogram_tester_.ExpectTotalCount(kOmniboxOpensHistogramName, 0);

  for (size_t i = 0; i < 297; i++) {
    ai_chat_metrics_->RecordOmniboxSearchQuery();
  }
  histogram_tester_.ExpectTotalCount(kOmniboxOpensHistogramName, 0);

  ai_chat_metrics_->RecordEnabled(true, true, GetPremiumCallback());
  histogram_tester_.ExpectUniqueSample(kOmniboxOpensHistogramName, 0, 1);

  ai_chat_metrics_->RecordOmniboxOpen();
  histogram_tester_.ExpectBucketCount(kOmniboxOpensHistogramName, 1, 1);

  for (size_t i = 0; i < 2; i++) {
    ai_chat_metrics_->RecordOmniboxOpen();
  }
  histogram_tester_.ExpectBucketCount(kOmniboxOpensHistogramName, 1, 2);
  histogram_tester_.ExpectBucketCount(kOmniboxOpensHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(3));
  histogram_tester_.ExpectBucketCount(kOmniboxOpensHistogramName, 2, 4);

  for (size_t i = 0; i < 12; i++) {
    ai_chat_metrics_->RecordOmniboxOpen();
  }
  histogram_tester_.ExpectBucketCount(kOmniboxOpensHistogramName, 2, 13);
  histogram_tester_.ExpectBucketCount(kOmniboxOpensHistogramName, 3, 3);

  histogram_tester_.ExpectBucketCount(kOmniboxOpensHistogramName, 0, 1);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectBucketCount(kOmniboxOpensHistogramName, 0, 2);
}

TEST_F(AIChatMetricsUnitTest, OmniboxWeekCompare) {
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());
  histogram_tester_.ExpectTotalCount(kOmniboxWeekCompareHistogramName, 0);

  for (size_t i = 0; i < 10; i++) {
    ai_chat_metrics_->RecordOmniboxSearchQuery();
  }
  histogram_tester_.ExpectTotalCount(kOmniboxWeekCompareHistogramName, 0);

  ai_chat_metrics_->RecordEnabled(true, true, GetPremiumCallback());
  for (size_t i = 0; i < 2; i++) {
    ai_chat_metrics_->RecordOmniboxOpen();
  }
  histogram_tester_.ExpectTotalCount(kOmniboxWeekCompareHistogramName, 0);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_.ExpectUniqueSample(kOmniboxWeekCompareHistogramName, 0, 1);

  for (size_t i = 0; i < 10; i++) {
    ai_chat_metrics_->RecordOmniboxSearchQuery();
  }
  for (size_t i = 0; i < 3; i++) {
    ai_chat_metrics_->RecordOmniboxOpen();
  }
  histogram_tester_.ExpectBucketCount(kOmniboxWeekCompareHistogramName, 0, 13);
  histogram_tester_.ExpectBucketCount(kOmniboxWeekCompareHistogramName, 1, 1);
  histogram_tester_.ExpectTotalCount(kOmniboxWeekCompareHistogramName, 14);
}

TEST_F(AIChatMetricsUnitTest, MostUsedContextMenuAction) {
  ai_chat_metrics_->RecordContextMenuUsage(ContextMenuAction::kSummarize);
  ai_chat_metrics_->RecordContextMenuUsage(ContextMenuAction::kExplain);
  ai_chat_metrics_->RecordContextMenuUsage(ContextMenuAction::kExplain);
  ai_chat_metrics_->RecordContextMenuUsage(ContextMenuAction::kParaphrase);

  histogram_tester_.ExpectTotalCount(kMostUsedContextMenuActionHistogramName,
                                     0);

  ai_chat_metrics_->RecordEnabled(true, true, GetPremiumCallback());

  histogram_tester_.ExpectUniqueSample(
      kMostUsedContextMenuActionHistogramName,
      static_cast<int>(ContextMenuAction::kExplain), 1);

  ai_chat_metrics_->RecordContextMenuUsage(ContextMenuAction::kImprove);

  histogram_tester_.ExpectBucketCount(
      kMostUsedContextMenuActionHistogramName,
      static_cast<int>(ContextMenuAction::kImprove), 0);
  ai_chat_metrics_->RecordContextMenuUsage(ContextMenuAction::kImprove);
  ai_chat_metrics_->RecordContextMenuUsage(ContextMenuAction::kImprove);

  histogram_tester_.ExpectBucketCount(
      kMostUsedContextMenuActionHistogramName,
      static_cast<int>(ContextMenuAction::kImprove), 1);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kMostUsedContextMenuActionHistogramName,
                                     10);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kMostUsedContextMenuActionHistogramName,
                                     10);
}

TEST_F(AIChatMetricsUnitTest, MostUsedEntryPoint) {
  ai_chat_metrics_->HandleOpenViaEntryPoint(EntryPoint::kOmniboxItem);
  ai_chat_metrics_->HandleOpenViaEntryPoint(EntryPoint::kSidebar);
  ai_chat_metrics_->HandleOpenViaEntryPoint(EntryPoint::kSidebar);
  ai_chat_metrics_->HandleOpenViaEntryPoint(EntryPoint::kContextMenu);

  histogram_tester_.ExpectTotalCount(kMostUsedEntryPointHistogramName, 0);

  ai_chat_metrics_->RecordEnabled(true, true, GetPremiumCallback());

  histogram_tester_.ExpectUniqueSample(kMostUsedEntryPointHistogramName,
                                       static_cast<int>(EntryPoint::kSidebar),
                                       1);

  ai_chat_metrics_->HandleOpenViaEntryPoint(EntryPoint::kToolbarButton);

  histogram_tester_.ExpectBucketCount(
      kMostUsedEntryPointHistogramName,
      static_cast<int>(EntryPoint::kToolbarButton), 0);
  ai_chat_metrics_->HandleOpenViaEntryPoint(EntryPoint::kToolbarButton);
  ai_chat_metrics_->HandleOpenViaEntryPoint(EntryPoint::kToolbarButton);

  histogram_tester_.ExpectBucketCount(
      kMostUsedEntryPointHistogramName,
      static_cast<int>(EntryPoint::kToolbarButton), 1);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kMostUsedEntryPointHistogramName, 10);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kMostUsedEntryPointHistogramName, 10);
}

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

TEST_F(AIChatMetricsUnitTest, Reset) {
  ai_chat_metrics_->RecordReset();
  histogram_tester_.ExpectUniqueSample(kAcquisitionSourceHistogramName,
                                       std::numeric_limits<int>::max() - 1, 1);
  histogram_tester_.ExpectUniqueSample(kEnabledHistogramName,
                                       std::numeric_limits<int>::max() - 1, 1);
}

}  // namespace ai_chat
