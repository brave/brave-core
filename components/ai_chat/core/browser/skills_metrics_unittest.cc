/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/skills_metrics.h"

#include <map>
#include <optional>
#include <string>

#include "base/containers/map_util.h"
#include "base/json/values_util.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AIChatSkillsMetricsUnitTest : public testing::Test {
 public:
  AIChatSkillsMetricsUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    SkillsMetrics::RegisterPrefs(local_state_.registry());
    local_state_.registry()->RegisterDictionaryPref(prefs::kBraveAIChatSkills);
    profile_prefs_.registry()->RegisterDictionaryPref(
        prefs::kBraveAIChatSkills);

    skills_metrics_ = std::make_unique<SkillsMetrics>(
        &local_state_, &profile_prefs_, &delegate_);
    skills_metrics_->ReportAllMetrics();
  }

 protected:
  class MockDelegate : public SkillsMetrics::Delegate {
   public:
    MockDelegate() = default;
    ~MockDelegate() override = default;

    uint64_t GetWeekChatCount() override { return week_chat_count; }

    base::Time GetConversationStartTime(
        const std::string& conversation_uuid) override {
      auto* start_time =
          base::FindOrNull(conversation_start_times, conversation_uuid);
      return start_time ? *start_time : base::Time::Now();
    }

    void Reset() {
      week_chat_count = 0;
      conversation_start_times.clear();
    }

    uint64_t week_chat_count = 0;
    std::map<std::string, base::Time> conversation_start_times;
  };

  void RecordNewPrompt(
      const std::string& conversation_uuid,
      std::optional<std::string> skill_shortcut = std::nullopt) {
    auto* start_time =
        base::FindOrNull(delegate_.conversation_start_times, conversation_uuid);
    bool is_new_chat = !start_time;

    auto turn = mojom::ConversationTurn::New();
    turn->uuid = conversation_uuid;
    turn->character_type = mojom::CharacterType::HUMAN;
    turn->text = "test";

    if (skill_shortcut) {
      auto skill = mojom::SkillEntry::New();
      skill->shortcut = *skill_shortcut;
      turn->skill = std::move(skill);
    }

    if (is_new_chat) {
      delegate_.week_chat_count++;
      delegate_.conversation_start_times[conversation_uuid] = base::Time::Now();
    }

    skills_metrics_->MaybeRecordNewPrompt(turn, conversation_uuid, is_new_chat);
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;
  MockDelegate delegate_;
  base::HistogramTester histogram_tester_;
  std::unique_ptr<SkillsMetrics> skills_metrics_;
};

TEST_F(AIChatSkillsMetricsUnitTest, WeeklySessions) {
  histogram_tester_.ExpectTotalCount(kSkillsWeeklySessionsHistogramName, 0);

  RecordNewPrompt("conv1", std::nullopt);
  RecordNewPrompt("conv3", std::nullopt);
  histogram_tester_.ExpectTotalCount(kSkillsWeeklySessionsHistogramName, 0);

  RecordNewPrompt("conv1", "skill1");
  RecordNewPrompt("conv1", "skill2");
  histogram_tester_.ExpectUniqueSample(kSkillsWeeklySessionsHistogramName, 1,
                                       2);

  RecordNewPrompt("conv2", "skill1");
  histogram_tester_.ExpectBucketCount(kSkillsWeeklySessionsHistogramName, 2, 1);
  histogram_tester_.ExpectTotalCount(kSkillsWeeklySessionsHistogramName, 3);

  task_environment_.FastForwardBy(base::Days(8));

  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectTotalCount(kSkillsWeeklySessionsHistogramName, 3);
}

TEST_F(AIChatSkillsMetricsUnitTest, SkillEntryPoint) {
  histogram_tester_.ExpectTotalCount(kSkillsEntryPointHistogramName, 0);

  RecordNewPrompt("conv1", std::nullopt);
  histogram_tester_.ExpectTotalCount(kSkillsEntryPointHistogramName, 0);

  skills_metrics_->RecordSkillClick("skill3");
  // Does not match clicked skill, assume implicit
  RecordNewPrompt("conv1", "skill1");
  RecordNewPrompt("conv1", "skill2");
  histogram_tester_.ExpectUniqueSample(kSkillsEntryPointHistogramName, 1, 2);

  for (size_t i = 0; i < 2; i++) {
    skills_metrics_->RecordSkillClick("skill1");
    RecordNewPrompt("conv1", "skill1");
  }
  histogram_tester_.ExpectUniqueSample(kSkillsEntryPointHistogramName, 1, 4);
  skills_metrics_->RecordSkillClick("skill1");
  RecordNewPrompt("conv1", "skill1");

  histogram_tester_.ExpectBucketCount(kSkillsEntryPointHistogramName, 2, 1);

  histogram_tester_.ExpectTotalCount(kSkillsEntryPointHistogramName, 5);

  task_environment_.FastForwardBy(base::Days(8));

  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectTotalCount(kSkillsEntryPointHistogramName, 5);
}

TEST_F(AIChatSkillsMetricsUnitTest, SkillAvgPrompts) {
  histogram_tester_.ExpectTotalCount(kSkillAvgPromptsHistogramName, 0);

  RecordNewPrompt("conv1", std::nullopt);
  histogram_tester_.ExpectTotalCount(kSkillAvgPromptsHistogramName, 0);

  for (size_t i = 0; i < 3; i++) {
    RecordNewPrompt("conv1", "skill1");
  }
  histogram_tester_.ExpectUniqueSample(kSkillAvgPromptsHistogramName, 1, 3);

  RecordNewPrompt("conv1", "skill1");
  histogram_tester_.ExpectBucketCount(kSkillAvgPromptsHistogramName, 2, 1);

  RecordNewPrompt("conv2", "skill1");
  // Brings average down to 2.5
  histogram_tester_.ExpectBucketCount(kSkillAvgPromptsHistogramName, 1, 4);
  histogram_tester_.ExpectTotalCount(kSkillAvgPromptsHistogramName, 5);

  task_environment_.FastForwardBy(base::Days(8));

  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectTotalCount(kSkillAvgPromptsHistogramName, 5);
}

TEST_F(AIChatSkillsMetricsUnitTest, PercentChatsWithSkill) {
  histogram_tester_.ExpectTotalCount(kPercentChatsWithSkillHistogramName, 0);

  RecordNewPrompt("conv1", std::nullopt);
  histogram_tester_.ExpectUniqueSample(kPercentChatsWithSkillHistogramName, 0,
                                       1);

  RecordNewPrompt("conv2", "skill1");
  // Should be 50%
  histogram_tester_.ExpectBucketCount(kPercentChatsWithSkillHistogramName, 2,
                                      1);

  RecordNewPrompt("conv2", "skill1");
  // Still 50%, this is not a new chat
  histogram_tester_.ExpectBucketCount(kPercentChatsWithSkillHistogramName, 2,
                                      2);

  RecordNewPrompt("conv3", "skill1");
  // Should be 66.67%
  histogram_tester_.ExpectBucketCount(kPercentChatsWithSkillHistogramName, 3,
                                      1);

  histogram_tester_.ExpectTotalCount(kPercentChatsWithSkillHistogramName, 4);

  task_environment_.FastForwardBy(base::Days(8));
  delegate_.Reset();

  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectTotalCount(kPercentChatsWithSkillHistogramName, 4);

  RecordNewPrompt("conv1", std::nullopt);
  histogram_tester_.ExpectBucketCount(kPercentChatsWithSkillHistogramName, 0,
                                      2);
  histogram_tester_.ExpectTotalCount(kPercentChatsWithSkillHistogramName, 5);
}

TEST_F(AIChatSkillsMetricsUnitTest, MonthlyPrompts) {
  histogram_tester_.ExpectTotalCount(kSkillMonthlyPromptsHistogramName, 0);

  for (size_t i = 0; i < 3; i++) {
    RecordNewPrompt("conv1", std::nullopt);
  }
  histogram_tester_.ExpectTotalCount(kSkillMonthlyPromptsHistogramName, 0);

  RecordNewPrompt("conv1", "skill1");
  histogram_tester_.ExpectUniqueSample(kSkillMonthlyPromptsHistogramName, 1, 1);

  RecordNewPrompt("conv1", "skill1");
  histogram_tester_.ExpectBucketCount(kSkillMonthlyPromptsHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(15));
  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectBucketCount(kSkillMonthlyPromptsHistogramName, 2, 2);

  task_environment_.FastForwardBy(base::Days(16));
  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectTotalCount(kSkillMonthlyPromptsHistogramName, 3);
}

TEST_F(AIChatSkillsMetricsUnitTest, SessionDurationWithSkill) {
  histogram_tester_.ExpectTotalCount(kSessionDurationWithSkillHistogramName, 0);

  RecordNewPrompt("conv1", std::nullopt);
  task_environment_.FastForwardBy(base::Minutes(2));
  RecordNewPrompt("conv1", std::nullopt);
  histogram_tester_.ExpectTotalCount(kSessionDurationWithSkillHistogramName, 0);

  RecordNewPrompt("conv2", std::nullopt);
  task_environment_.FastForwardBy(base::Minutes(2) + base::Seconds(30));
  RecordNewPrompt("conv2", "skill1");
  histogram_tester_.ExpectUniqueSample(kSessionDurationWithSkillHistogramName,
                                       1, 1);

  task_environment_.FastForwardBy(base::Seconds(15));
  RecordNewPrompt("conv2", std::nullopt);
  histogram_tester_.ExpectUniqueSample(kSessionDurationWithSkillHistogramName,
                                       1, 2);

  task_environment_.FastForwardBy(base::Minutes(1));
  RecordNewPrompt("conv2", "skill2");
  // Should be 3.75 minutes
  histogram_tester_.ExpectBucketCount(kSessionDurationWithSkillHistogramName, 2,
                                      1);

  RecordNewPrompt("conv3", "skill1");
  // Average should be 1.875 minutes
  histogram_tester_.ExpectBucketCount(kSessionDurationWithSkillHistogramName, 1,
                                      3);
  histogram_tester_.ExpectTotalCount(kSessionDurationWithSkillHistogramName, 4);

  task_environment_.FastForwardBy(base::Days(8));

  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectTotalCount(kSessionDurationWithSkillHistogramName, 4);
}

TEST_F(AIChatSkillsMetricsUnitTest, SkillsLastEngagementTime) {
  histogram_tester_.ExpectTotalCount(kSkillsLastEngagementTimeHistogramName, 0);

  RecordNewPrompt("conv1", std::nullopt);
  histogram_tester_.ExpectTotalCount(kSkillsLastEngagementTimeHistogramName, 0);

  RecordNewPrompt("conv1", "skill1");
  histogram_tester_.ExpectUniqueSample(kSkillsLastEngagementTimeHistogramName,
                                       1, 1);

  task_environment_.FastForwardBy(base::Days(8));
  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectBucketCount(kSkillsLastEngagementTimeHistogramName, 2,
                                      1);

  task_environment_.FastForwardBy(base::Days(8));
  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectBucketCount(kSkillsLastEngagementTimeHistogramName, 3,
                                      1);

  RecordNewPrompt("conv2", "skill1");
  histogram_tester_.ExpectBucketCount(kSkillsLastEngagementTimeHistogramName, 1,
                                      2);

  histogram_tester_.ExpectTotalCount(kSkillsLastEngagementTimeHistogramName, 4);

  task_environment_.FastForwardBy(base::Days(65));
  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectBucketCount(kSkillsLastEngagementTimeHistogramName, 6,
                                      1);
  histogram_tester_.ExpectTotalCount(kSkillsLastEngagementTimeHistogramName, 5);
}

TEST_F(AIChatSkillsMetricsUnitTest, SkillsCount) {
  histogram_tester_.ExpectTotalCount(kSkillsCountHistogramName, 0);

  skills_metrics_->NotifySkillChange();
  histogram_tester_.ExpectTotalCount(kSkillsCountHistogramName, 0);

  prefs::AddSkillToPrefs("skill1", "prompt 1", std::nullopt, profile_prefs_);
  prefs::AddSkillToPrefs("skill2", "prompt 2", std::nullopt, profile_prefs_);
  prefs::AddSkillToPrefs("skill3", "prompt 3", std::nullopt, profile_prefs_);

  skills_metrics_->NotifySkillChange();
  histogram_tester_.ExpectUniqueSample(kSkillsCountHistogramName, 1, 1);

  prefs::AddSkillToPrefs("skill4", "prompt 2", std::nullopt, profile_prefs_);

  skills_metrics_->NotifySkillChange();
  histogram_tester_.ExpectBucketCount(kSkillsCountHistogramName, 2, 1);

  {
    ScopedDictPrefUpdate update(&profile_prefs_, prefs::kBraveAIChatSkills);
    update->Remove(update->begin()->first);
  }

  skills_metrics_->NotifySkillChange();
  histogram_tester_.ExpectBucketCount(kSkillsCountHistogramName, 1, 2);
  histogram_tester_.ExpectTotalCount(kSkillsCountHistogramName, 3);

  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectBucketCount(kSkillsCountHistogramName, 1, 3);
  histogram_tester_.ExpectTotalCount(kSkillsCountHistogramName, 4);
}

TEST_F(AIChatSkillsMetricsUnitTest, SkillsUsed) {
  histogram_tester_.ExpectTotalCount(kSkillsUsedHistogramName, 0);

  RecordNewPrompt("conv1", std::nullopt);
  histogram_tester_.ExpectTotalCount(kSkillsUsedHistogramName, 0);

  RecordNewPrompt("conv1", "skill1");
  RecordNewPrompt("conv2", "skill2");
  RecordNewPrompt("conv3", "skill1");
  RecordNewPrompt("conv4", "skill3");
  RecordNewPrompt("conv3", "skill1");
  histogram_tester_.ExpectUniqueSample(kSkillsUsedHistogramName, 1, 5);

  RecordNewPrompt("conv3", "skill4");
  histogram_tester_.ExpectBucketCount(kSkillsUsedHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(4));
  RecordNewPrompt("conv3", "skill4");
  histogram_tester_.ExpectBucketCount(kSkillsUsedHistogramName, 2, 2);

  task_environment_.FastForwardBy(base::Days(6));
  skills_metrics_->ReportAllMetrics();
  histogram_tester_.ExpectBucketCount(kSkillsUsedHistogramName, 1, 6);

  histogram_tester_.ExpectTotalCount(kSkillsUsedHistogramName, 8);
  task_environment_.FastForwardBy(base::Days(7));
  skills_metrics_->ReportAllMetrics();

  histogram_tester_.ExpectTotalCount(kSkillsUsedHistogramName, 8);
}

}  // namespace ai_chat
