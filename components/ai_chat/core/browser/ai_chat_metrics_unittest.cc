/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"

#include <stddef.h>

#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "base/numerics/clamped_math.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

class AIChatMetricsUnitTest : public testing::Test {
 public:
  AIChatMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    auto* registry = local_state_.registry();
    AIChatMetrics::RegisterPrefs(registry);
    profile_prefs_.registry()->RegisterBooleanPref(
        prefs::kBraveAIChatTabOrganizationEnabled, false);
    task_environment_.FastForwardBy(base::Days(30));
    ai_chat_metrics_ =
        std::make_unique<AIChatMetrics>(&local_state_, &profile_prefs_);
  }

  void RecordPrompts(std::string id, size_t prompt_count) {
    auto conversation_info = CreateConversationAndTurn(id, "test");

    for (size_t j = 0; j < prompt_count; j++) {
      conversation_handler_.current_history_size_++;
      ai_chat_metrics_->RecordNewPrompt(&conversation_handler_,
                                        conversation_info.first,
                                        conversation_info.second);
    }
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
  class MockConversationHandler : public ConversationHandlerForMetrics {
   public:
    MockConversationHandler() = default;
    ~MockConversationHandler() override = default;

    size_t GetConversationHistorySize() override {
      return current_history_size_;
    }
    bool should_send_page_contents() const override {
      return should_send_page_contents_;
    }
    mojom::APIError current_error() const override { return current_error_; }

    size_t current_history_size_ = 0;
    bool should_send_page_contents_ = false;
    mojom::APIError current_error_ = mojom::APIError::None;
  };

  std::pair<mojom::ConversationPtr, mojom::ConversationTurnPtr>
  CreateConversationAndTurn(const std::string& uuid,
                            const std::string& turn_text) {
    auto turn = mojom::ConversationTurn::New();
    turn->uuid = uuid;
    turn->character_type = mojom::CharacterType::HUMAN;
    turn->action_type = mojom::ActionType::UNSPECIFIED;
    turn->text = turn_text;
    turn->created_time = base::Time::Now();
    turn->from_brave_search_SERP = false;

    auto conversation = mojom::Conversation::New();
    conversation->uuid = uuid;
    conversation->updated_time = turn->created_time;
    conversation->has_content = true;

    return {std::move(conversation), std::move(turn)};
  }

  MockConversationHandler conversation_handler_;
  bool is_premium_ = false;
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
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
  ai_chat_metrics_->OnPremiumStatusUpdated(
      true, false, mojom::PremiumStatus::Active, nullptr);
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

  RecordPrompts("abc", 5);
  task_environment_.FastForwardBy(base::Seconds(5));
  histogram_tester_.ExpectUniqueSample(kChatCountHistogramName, 0, 1);

  RecordPrompts("def1", 5);
  RecordPrompts("def2", 5);
  RecordPrompts("def3", 5);
  RecordPrompts("def4", 5);
  task_environment_.FastForwardBy(base::Seconds(5));
  histogram_tester_.ExpectBucketCount(kChatCountHistogramName, 1, 1);

  RecordPrompts("xyz1", 3);
  task_environment_.FastForwardBy(base::Seconds(5));
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

  RecordPrompts("abc", 1);
  task_environment_.FastForwardBy(base::Seconds(5));
  histogram_tester_.ExpectUniqueSample(kAvgPromptCountHistogramName, 0, 1);

  RecordPrompts("abc", 2);
  task_environment_.FastForwardBy(base::Seconds(5));
  histogram_tester_.ExpectBucketCount(kAvgPromptCountHistogramName, 1, 1);

  RecordPrompts("abc", 4);
  task_environment_.FastForwardBy(base::Seconds(5));
  histogram_tester_.ExpectBucketCount(kAvgPromptCountHistogramName, 2, 1);

  RecordPrompts("def", 1);
  task_environment_.FastForwardBy(base::Seconds(5));
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

  RecordPrompts("abc", 1);
  task_environment_.FastForwardBy(base::Seconds(5));
  histogram_tester_.ExpectUniqueSample(kUsageDailyHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kUsageWeeklyHistogramName, 1, 1);
  histogram_tester_.ExpectUniqueSample(kUsageMonthlyHistogramName, 1, 1);

  is_premium_ = true;
  ai_chat_metrics_->OnPremiumStatusUpdated(
      true, false, mojom::PremiumStatus::Active, nullptr);
  RecordPrompts("def", 1);
  task_environment_.FastForwardBy(base::Seconds(5));
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

  RecordPrompts("abc", 1);
  task_environment_.FastForwardBy(base::Seconds(5));
  histogram_tester_.ExpectUniqueSample(kNewUserReturningHistogramName, 1, 2);
}

TEST_F(AIChatMetricsUnitTest, FeatureUsage) {
  ai_chat_metrics_->RecordEnabled(false, false, GetPremiumCallback());
  histogram_tester_.ExpectUniqueSample(kNewUserReturningHistogramName, 0, 1);

  ai_chat_metrics_->RecordEnabled(true, true, GetPremiumCallback());
  histogram_tester_.ExpectUniqueSample(kNewUserReturningHistogramName, 0, 2);
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 0);

  RecordPrompts("abc", 1);
  task_environment_.FastForwardBy(base::Seconds(5));
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

  for (size_t i = 0; i < 4; i++) {
    ai_chat_metrics_->HandleOpenViaEntryPoint(EntryPoint::kBraveSearch);
  }

  histogram_tester_.ExpectBucketCount(
      kMostUsedEntryPointHistogramName,
      static_cast<int>(EntryPoint::kBraveSearch), 1);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kMostUsedEntryPointHistogramName, 14);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kMostUsedEntryPointHistogramName, 14);
}

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

TEST_F(AIChatMetricsUnitTest, Reset) {
  ai_chat_metrics_->RecordReset();
  histogram_tester_.ExpectUniqueSample(kAcquisitionSourceHistogramName,
                                       std::numeric_limits<int>::max() - 1, 1);
  histogram_tester_.ExpectUniqueSample(kEnabledHistogramName,
                                       std::numeric_limits<int>::max() - 1, 1);
}

TEST_F(AIChatMetricsUnitTest, ChatHistory) {
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());

  // Record some prompts with single turns
  RecordPrompts("chat1", 1);
  task_environment_.FastForwardBy(base::Seconds(5));
  histogram_tester_.ExpectTotalCount(kChatHistoryUsageHistogramName, 1);
  histogram_tester_.ExpectUniqueSample(kChatHistoryUsageHistogramName, 0, 1);

  conversation_handler_.current_history_size_ = 1;
  RecordPrompts("chat1", 1);
  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_.ExpectTotalCount(kChatHistoryUsageHistogramName, 2);
  histogram_tester_.ExpectUniqueSample(kChatHistoryUsageHistogramName, 0, 2);

  ai_chat_metrics_->RecordConversationUnload("chat1");

  conversation_handler_.current_history_size_ = 1;
  RecordPrompts("chat1", 1);
  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_.ExpectTotalCount(kChatHistoryUsageHistogramName, 3);
  histogram_tester_.ExpectBucketCount(kChatHistoryUsageHistogramName, 5, 1);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kChatHistoryUsageHistogramName, 9);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kChatHistoryUsageHistogramName, 9);
}

TEST_F(AIChatMetricsUnitTest, ChatDuration) {
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());

  task_environment_.FastForwardBy(base::Seconds(30));

  RecordPrompts("chat1", 1);
  task_environment_.FastForwardBy(base::Seconds(5));
  histogram_tester_.ExpectUniqueSample(kMaxChatDurationHistogramName, 0, 1);

  task_environment_.FastForwardBy(base::Minutes(29));

  RecordPrompts("chat1", 1);
  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_.ExpectBucketCount(kMaxChatDurationHistogramName, 4, 1);
  histogram_tester_.ExpectTotalCount(kMaxChatDurationHistogramName, 2);

  task_environment_.FastForwardBy(base::Minutes(15));
  RecordPrompts("chat1", 1);
  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_.ExpectBucketCount(kMaxChatDurationHistogramName, 5, 1);
  histogram_tester_.ExpectTotalCount(kMaxChatDurationHistogramName, 3);

  ai_chat_metrics_->RecordConversationUnload("chat1");

  task_environment_.FastForwardBy(base::Minutes(60));
  RecordPrompts("chat1", 1);
  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_.ExpectBucketCount(kMaxChatDurationHistogramName, 5, 2);
  histogram_tester_.ExpectTotalCount(kMaxChatDurationHistogramName, 4);

  task_environment_.FastForwardBy(base::Days(7));
  auto total_count =
      histogram_tester_.GetTotalCountsForPrefix(kMaxChatDurationHistogramName)
          .begin()
          ->second;
  EXPECT_GE(total_count, 10);
  EXPECT_LE(total_count, 11);

  task_environment_.FastForwardBy(base::Days(7));
  EXPECT_EQ(
      histogram_tester_.GetTotalCountsForPrefix(kMaxChatDurationHistogramName)
          .begin()
          ->second,
      total_count);
}

TEST_F(AIChatMetricsUnitTest, FirstChatPrompts) {
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());

  histogram_tester_.ExpectTotalCount(kFirstChatPromptsHistogramName, 0);

  RecordPrompts("chat1", 4);
  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_.ExpectTotalCount(kFirstChatPromptsHistogramName, 0);

  ai_chat_metrics_->RecordConversationUnload("chat1");
  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_.ExpectUniqueSample(kFirstChatPromptsHistogramName, 2, 1);

  ai_chat_metrics_->RecordConversationUnload("chat1");
  RecordPrompts("chat1", 30);
  task_environment_.FastForwardBy(base::Seconds(5));

  histogram_tester_.ExpectUniqueSample(kFirstChatPromptsHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kFirstChatPromptsHistogramName, 1);
}

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
TEST_F(AIChatMetricsUnitTest, FullPageSwitch) {
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());

  ai_chat_metrics_->RecordSidebarUsage();
  ai_chat_metrics_->RecordSidebarUsage();

  histogram_tester_.ExpectUniqueSample(kFullPageSwitchesHistogramName, 0, 2);

  ai_chat_metrics_->RecordFullPageSwitch();
  ai_chat_metrics_->RecordFullPageSwitch();

  histogram_tester_.ExpectBucketCount(kFullPageSwitchesHistogramName, 3, 1);
  histogram_tester_.ExpectBucketCount(kFullPageSwitchesHistogramName, 4, 1);
  histogram_tester_.ExpectTotalCount(kFullPageSwitchesHistogramName, 4);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kFullPageSwitchesHistogramName, 10);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kFullPageSwitchesHistogramName, 10);
}
#endif

TEST_F(AIChatMetricsUnitTest, ContextSource) {
  ai_chat_metrics_->RecordEnabled(true, false, GetPremiumCallback());

  histogram_tester_.ExpectTotalCount(kMostUsedContextSourceHistogramName, 0);

  // Test conversation starter context when history size > 1
  conversation_handler_.current_history_size_ = 2;
  auto chat = CreateConversationAndTurn("chat10", "hello");
  chat.second->action_type = mojom::ActionType::CONVERSATION_STARTER;
  ai_chat_metrics_->RecordNewPrompt(&conversation_handler_, chat.first,
                                    chat.second);
  histogram_tester_.ExpectUniqueSample(kMostUsedContextSourceHistogramName, 4,
                                       1);
  histogram_tester_.ExpectTotalCount(kMostUsedContextSourceHistogramName, 1);

  conversation_handler_.current_history_size_ = 1;
  // Test conversation starter context
  chat = CreateConversationAndTurn("chat2", "hello");
  chat.second->action_type = mojom::ActionType::CONVERSATION_STARTER;
  ai_chat_metrics_->RecordNewPrompt(&conversation_handler_, chat.first,
                                    chat.second);
  ai_chat_metrics_->RecordNewPrompt(&conversation_handler_, chat.first,
                                    chat.second);
  histogram_tester_.ExpectBucketCount(kMostUsedContextSourceHistogramName, 1,
                                      2);

  // Test page summary context
  chat = CreateConversationAndTurn("chat1", "test");
  chat.second->action_type = mojom::ActionType::SUMMARIZE_PAGE;
  for (int i = 0; i < 3; i++) {
    ai_chat_metrics_->RecordNewPrompt(&conversation_handler_, chat.first,
                                      chat.second);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextSourceHistogramName, 2,
                                      1);

  // Test text input with page context
  chat = CreateConversationAndTurn("chat4", "test");
  conversation_handler_.should_send_page_contents_ = true;
  for (int i = 0; i < 4; i++) {
    ai_chat_metrics_->RecordNewPrompt(&conversation_handler_, chat.first,
                                      chat.second);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextSourceHistogramName, 3,
                                      1);
  conversation_handler_.should_send_page_contents_ = false;

  // Test text input without page context (default)
  chat = CreateConversationAndTurn("chat6", "test");
  for (int i = 0; i < 4; i++) {
    ai_chat_metrics_->RecordNewPrompt(&conversation_handler_, chat.first,
                                      chat.second);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextSourceHistogramName, 4,
                                      2);

  // Test text input via full page context
  chat = CreateConversationAndTurn("chat5", "test");
  for (int i = 0; i < 6; i++) {
    ai_chat_metrics_->OnSendingPromptWithFullPage();
    ai_chat_metrics_->RecordNewPrompt(&conversation_handler_, chat.first,
                                      chat.second);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextSourceHistogramName, 5,
                                      1);

  // Test quick action context
  chat = CreateConversationAndTurn("chat3", "test");
  for (size_t i = 0; i < 7; i++) {
    ai_chat_metrics_->OnQuickActionStatusChange(true);
    ai_chat_metrics_->RecordNewPrompt(&conversation_handler_, chat.first,
                                      chat.second);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextSourceHistogramName, 6,
                                      1);

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  chat = CreateConversationAndTurn("chat2", "test");
  for (size_t i = 0; i < 7; i++) {
    ai_chat_metrics_->RecordOmniboxOpen();
    ai_chat_metrics_->RecordNewPrompt(&conversation_handler_, chat.first,
                                      chat.second);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextSourceHistogramName, 0,
                                      1);
#endif
}

TEST_F(AIChatMetricsUnitTest, RateLimitMetrics) {
  ai_chat_metrics_->RecordEnabled(true, true, GetPremiumCallback());

  histogram_tester_.ExpectTotalCount(kRateLimitStopsHistogramName, 0);

  RecordPrompts("chat1", 1);
  task_environment_.FastForwardBy(base::Seconds(5));
  histogram_tester_.ExpectUniqueSample(kRateLimitStopsHistogramName, 0, 1);

  conversation_handler_.current_error_ = mojom::APIError::RateLimitReached;
  ai_chat_metrics_->MaybeRecordLastError(&conversation_handler_);

  histogram_tester_.ExpectBucketCount(kRateLimitStopsHistogramName, 1, 1);
  histogram_tester_.ExpectTotalCount(kRateLimitStopsHistogramName, 2);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kRateLimitStopsHistogramName, 8);

  task_environment_.FastForwardBy(base::Days(7));
  histogram_tester_.ExpectTotalCount(kRateLimitStopsHistogramName, 8);
}

}  // namespace ai_chat
