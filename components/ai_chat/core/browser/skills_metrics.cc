/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/skills_metrics.h"

#include <string_view>

#include "base/json/values_util.h"
#include "base/metrics/histogram_macros.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace ai_chat {

namespace {

enum SkillsEntryPoint {
  kImplicit = 1,
  kExplicit = 2,
  kMaxValue = kExplicit,
};

constexpr int kSkillsWeeklySessionsBuckets[] = {0, 1, 5, 10, 20};
constexpr int kSkillAvgPromptsBuckets[] = {0, 3, 10, 20};
constexpr int kPercentChatsWithSkillBuckets[] = {0, 25, 50, 75, 100};
constexpr int kSkillMonthlyPromptsBuckets[] = {0, 1, 3, 10};
constexpr int kSessionDurationWithSkillBuckets[] = {1, 3, 10, 20};
constexpr int kSkillsCountBuckets[] = {0, 3, 10, 20};
constexpr int kSkillsUsedBuckets[] = {0, 3, 10, 20};

}  // namespace

SkillsMetrics::SkillsMetrics(PrefService* local_state,
                             PrefService* profile_prefs,
                             Delegate* delegate)
    : delegate_(delegate),
      local_state_(local_state),
      profile_prefs_(profile_prefs),
      skills_sessions_storage_(local_state,
                               prefs::kBraveChatP3ASkillsWeeklySessionsStorage),
      skill_explicit_selection_storage_(
          local_state,
          prefs::kBraveChatP3ASkillsExplicitSelectionStorage),
      skills_prompt_count_storage_(
          local_state,
          prefs::kBraveChatP3ASkillsPromptCountStorage),
      session_duration_with_skill_storage_(
          local_state,
          prefs::kBraveChatP3ASessionDurationWithSkillStorage) {
  CHECK(delegate_);
}

SkillsMetrics::~SkillsMetrics() = default;

void SkillsMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBraveChatP3ASkillsWeeklySessionsStorage);
  registry->RegisterListPref(
      prefs::kBraveChatP3ASkillsExplicitSelectionStorage);
  registry->RegisterListPref(prefs::kBraveChatP3ASkillsPromptCountStorage);
  registry->RegisterListPref(
      prefs::kBraveChatP3ASessionDurationWithSkillStorage);
  registry->RegisterTimePref(prefs::kBraveChatP3ASkillsLastEngagementTime, {});
  registry->RegisterDictionaryPref(prefs::kBraveChatP3ASkillsUsedStorage);
}

void SkillsMetrics::MaybeRecordNewPrompt(mojom::ConversationTurnPtr& entry,
                                         const std::string& conversation_uuid,
                                         bool is_new_chat) {
  bool conversation_known =
      conversations_with_skill_.contains(conversation_uuid);
  if (conversation_known) {
    // Update session duration of conversation that used a skill previously or
    // currently
    UpdateCachedSessionDuration(conversation_uuid);
  }

  if (!entry->skill) {
    if (is_new_chat) {
      ReportSkillChatPercentage();
    }
    if (conversation_known) {
      ReportSessionDurationWithSkill();
    }
    clicked_skill_shortcut_ = std::nullopt;
    return;
  }

  if (!conversation_known) {
    skills_sessions_storage_.AddDelta(1);
    UpdateCachedSessionDuration(conversation_uuid);
  }

  skills_prompt_count_storage_.AddDelta(1);

  if (entry->skill->shortcut == clicked_skill_shortcut_) {
    skill_explicit_selection_storage_.AddDelta(1);
  }

  if (!entry->skill->shortcut.empty()) {
    ScopedDictPrefUpdate update(local_state_,
                                prefs::kBraveChatP3ASkillsUsedStorage);
    update->Set(entry->skill->shortcut, base::TimeToValue(base::Time::Now()));
  }

  clicked_skill_shortcut_ = std::nullopt;

  local_state_->SetTime(prefs::kBraveChatP3ASkillsLastEngagementTime,
                        base::Time::Now());

  ReportAllMetrics();
}

void SkillsMetrics::UpdateCachedSessionDuration(
    const std::string& conversation_uuid) {
  auto start_time = delegate_->GetConversationStartTime(conversation_uuid);
  auto new_duration = base::Time::Now() - start_time;
  auto old_duration = conversations_with_skill_[conversation_uuid];

  session_duration_with_skill_storage_.SubDelta(old_duration.InSeconds());
  session_duration_with_skill_storage_.AddDelta(new_duration.InSeconds());
  conversations_with_skill_[conversation_uuid] = new_duration;
}

void SkillsMetrics::RecordSkillClick(const std::string& skill_shortcut) {
  clicked_skill_shortcut_ = skill_shortcut;
}

void SkillsMetrics::RecordConversationUnload(
    std::string_view conversation_uuid) {
  conversations_with_skill_.erase(conversation_uuid);
}

void SkillsMetrics::ReportAllMetrics() {
  ReportWeeklySessions();
  ReportSkillChatPercentage();
  ReportMonthlyPrompts();
  ReportSessionDurationWithSkill();
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_state_, prefs::kBraveChatP3ASkillsLastEngagementTime,
      kSkillsLastEngagementTimeHistogramName, false);
  ReportSkillsCount();
  ReportSkillsUsed();
}

void SkillsMetrics::NotifySkillChange() {
  ReportSkillsCount();
}

void SkillsMetrics::ReportWeeklySessions() {
  base::Time today_midnight = base::Time::Now().LocalMidnight();
  base::Time one_week_ago = today_midnight - base::Days(6);

  auto prompt_count = skills_prompt_count_storage_.GetPeriodSumInTimeRange(
      one_week_ago, today_midnight);

  if (prompt_count == 0) {
    return;
  }

  auto sessions_count = skills_sessions_storage_.GetWeeklySum();
  p3a_utils::RecordToHistogramBucket(kSkillsWeeklySessionsHistogramName,
                                     kSkillsWeeklySessionsBuckets,
                                     sessions_count);

  auto explicit_count = skill_explicit_selection_storage_.GetWeeklySum();
  auto implicit_count = prompt_count - explicit_count;

  if (explicit_count > implicit_count) {
    UMA_HISTOGRAM_ENUMERATION(kSkillsEntryPointHistogramName, kExplicit);
  } else {
    UMA_HISTOGRAM_ENUMERATION(kSkillsEntryPointHistogramName, kImplicit);
  }

  if (sessions_count > 0) {
    int avg_prompts = static_cast<int>(
        std::ceil(static_cast<double>(prompt_count) / sessions_count));
    p3a_utils::RecordToHistogramBucket(kSkillAvgPromptsHistogramName,
                                       kSkillAvgPromptsBuckets, avg_prompts);
  }
}

void SkillsMetrics::ReportSkillChatPercentage() {
  auto chat_count = delegate_->GetWeekChatCount();
  if (chat_count == 0) {
    return;
  }

  auto chat_with_skill_count = skills_sessions_storage_.GetWeeklySum();

  int percentage =
      static_cast<int>(std::round(100.0 * chat_with_skill_count / chat_count));

  p3a_utils::RecordToHistogramBucket(kPercentChatsWithSkillHistogramName,
                                     kPercentChatsWithSkillBuckets, percentage);
}

void SkillsMetrics::ReportMonthlyPrompts() {
  auto prompt_count = skills_prompt_count_storage_.GetMonthlySum();

  if (prompt_count == 0) {
    return;
  }

  p3a_utils::RecordToHistogramBucket(kSkillMonthlyPromptsHistogramName,
                                     kSkillMonthlyPromptsBuckets, prompt_count);
}

void SkillsMetrics::ReportSessionDurationWithSkill() {
  auto total_duration_seconds =
      session_duration_with_skill_storage_.GetWeeklySum();
  auto sessions_count = skills_sessions_storage_.GetWeeklySum();
  if (sessions_count == 0) {
    return;
  }

  int avg_duration_minutes = static_cast<int>(std::ceil(
      static_cast<double>(total_duration_seconds) / sessions_count / 60));

  p3a_utils::RecordToHistogramBucket(kSessionDurationWithSkillHistogramName,
                                     kSessionDurationWithSkillBuckets,
                                     avg_duration_minutes);
}

void SkillsMetrics::ReportSkillsCount() {
  const auto& skills_dict = profile_prefs_->GetDict(prefs::kBraveAIChatSkills);
  int skills_count = skills_dict.size();

  if (skills_count == 0) {
    return;
  }

  p3a_utils::RecordToHistogramBucket(kSkillsCountHistogramName,
                                     kSkillsCountBuckets, skills_count);
}

void SkillsMetrics::ReportSkillsUsed() {
  base::Time one_week_ago = base::Time::Now() - base::Days(7);

  ScopedDictPrefUpdate update(local_state_,
                              prefs::kBraveChatP3ASkillsUsedStorage);
  auto& skills_dict = update.Get();

  std::vector<std::string> keys_to_remove;
  for (const auto [shortcut, timestamp_value] : skills_dict) {
    auto timestamp = base::ValueToTime(timestamp_value);
    if (!timestamp || *timestamp < one_week_ago) {
      keys_to_remove.push_back(shortcut);
    }
  }

  for (const auto& shortcut : keys_to_remove) {
    skills_dict.Remove(shortcut);
  }

  int unique_skills_count = skills_dict.size();
  if (unique_skills_count == 0) {
    return;
  }

  p3a_utils::RecordToHistogramBucket(kSkillsUsedHistogramName,
                                     kSkillsUsedBuckets, unique_skills_count);
}

}  // namespace ai_chat
