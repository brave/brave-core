/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SKILLS_METRICS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SKILLS_METRICS_H_

#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/time_period_storage/monthly_storage.h"
#include "brave/components/time_period_storage/weekly_storage.h"

class PrefRegistrySimple;
class PrefService;

namespace ai_chat {

inline constexpr char kSkillsWeeklySessionsHistogramName[] =
    "Brave.AIChat.SkillsWeeklySessions";
inline constexpr char kSkillsEntryPointHistogramName[] =
    "Brave.AIChat.SkillsEntryPoint";
inline constexpr char kSkillAvgPromptsHistogramName[] =
    "Brave.AIChat.SkillAvgPrompts";
inline constexpr char kPercentChatsWithSkillHistogramName[] =
    "Brave.AIChat.PercentChatsWithSkill";
inline constexpr char kSkillMonthlyPromptsHistogramName[] =
    "Brave.AIChat.SkillMonthlyPrompts";
inline constexpr char kSessionDurationWithSkillHistogramName[] =
    "Brave.AIChat.SessionDurationWithSkill";
inline constexpr char kSkillsLastEngagementTimeHistogramName[] =
    "Brave.AIChat.SkillsLastEngagementTime";
inline constexpr char kSkillsCountHistogramName[] = "Brave.AIChat.SkillsCount";
inline constexpr char kSkillsUsedHistogramName[] = "Brave.AIChat.SkillsUsed";

class SkillsMetrics {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual uint64_t GetWeekChatCount() = 0;
    virtual base::Time GetConversationStartTime(
        const std::string& conversation_uuid) = 0;
  };

  explicit SkillsMetrics(PrefService* local_state,
                         PrefService* profile_prefs,
                         Delegate* delegate = nullptr);
  ~SkillsMetrics();

  SkillsMetrics(const SkillsMetrics&) = delete;
  SkillsMetrics& operator=(const SkillsMetrics&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void MaybeRecordNewPrompt(mojom::ConversationTurnPtr& entry,
                            const std::string& conversation_uuid,
                            bool is_new_chat);
  void RecordSkillClick(const std::string& skill_shortcut);
  void RecordConversationUnload(std::string_view conversation_uuid);
  void ReportAllMetrics();
  void NotifySkillChange();

 private:
  void UpdateCachedSessionDuration(const std::string& conversation_uuid);

  void ReportWeeklySessions();
  void ReportSkillChatPercentage();
  void ReportMonthlyPrompts();
  void ReportSessionDurationWithSkill();
  void ReportSkillsCount();
  void ReportSkillsUsed();

  raw_ptr<Delegate> delegate_;
  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;

  WeeklyStorage skills_sessions_storage_;
  WeeklyStorage skill_explicit_selection_storage_;
  MonthlyStorage skills_prompt_count_storage_;
  WeeklyStorage session_duration_with_skill_storage_;

  std::optional<std::string> clicked_skill_shortcut_;
  // Map of conversation UUID to session durations
  base::flat_map<std::string, base::TimeDelta> conversations_with_skill_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SKILLS_METRICS_H_
