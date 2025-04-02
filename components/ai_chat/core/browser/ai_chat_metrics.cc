/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"

#include <limits.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/metrics/histogram_functions_internal_overloads.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/sidebar/common/features.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

namespace {

using sidebar::features::SidebarDefaultMode;

constexpr base::TimeDelta kReportInterval = base::Hours(24);
constexpr base::TimeDelta kReportDebounceDelay = base::Seconds(3);
constexpr base::TimeDelta kFirstChatPromptsReportDebounceDelay =
    base::Minutes(10);

constexpr int kFirstChatPromptsBuckets[] = {1, 3, 6, 10};
constexpr int kChatCountBuckets[] = {1, 5, 10, 20, 50};
constexpr int kAvgPromptCountBuckets[] = {2, 5, 10, 20};
constexpr int kChatHistoryUsageBuckets[] = {0, 1, 4, 10, 25, 50, 75};
constexpr int kMaxChatDurationBuckets[] = {1, 2, 5, 15, 30, 60};
constexpr int kRateLimitsBuckets[] = {0, 1, 3, 5};
constexpr int kContextLimitsBuckets[] = {0, 2, 5, 10};

constexpr base::TimeDelta kPremiumCheckInterval = base::Days(1);

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
constexpr int kFullPageSwitchesBuckets[] = {0, 5, 25, 50};
// Value -1 is added to buckets to add padding for the "less than 1% option"
constexpr int kOmniboxOpenBuckets[] = {-1, 0, 3, 5, 10, 25};
constexpr int kContextMenuUsageBuckets[] = {0, 1, 2, 5, 10, 20, 50};

constexpr char kSummarizeActionKey[] = "summarize";
constexpr char kExplainActionKey[] = "explain";
constexpr char kParaphraseActionKey[] = "paraphrase";
constexpr char kCreateTaglineActionKey[] = "tagline";
constexpr char kCreateSocialMediaActionKey[] = "social";
constexpr char kImproveActionKey[] = "improve";
constexpr char kChangeToneActionKey[] = "tone";
constexpr char kChangeLengthActionKey[] = "length";

constexpr char kOmniboxItemEntryPointKey[] = "omnibox_item";
constexpr char kSidebarEntryPointKey[] = "sidebar";
constexpr char kContextMenuEntryPointKey[] = "context_menu";
constexpr char kToolbarButtonEntryPointKey[] = "toolbar_button";
constexpr char kMenuItemEntryPointKey[] = "menu_item";
constexpr char kOmniboxCommandEntryPointKey[] = "omnibox_command";
constexpr char kBraveSearchEntryPointKey[] = "brave_search";

constexpr auto kContextMenuActionKeys =
    base::MakeFixedFlatMap<ContextMenuAction, const char*>(
        {{ContextMenuAction::kSummarize, kSummarizeActionKey},
         {ContextMenuAction::kExplain, kExplainActionKey},
         {ContextMenuAction::kParaphrase, kParaphraseActionKey},
         {ContextMenuAction::kCreateTagline, kCreateTaglineActionKey},
         {ContextMenuAction::kCreateSocialMedia, kCreateSocialMediaActionKey},
         {ContextMenuAction::kImprove, kImproveActionKey},
         {ContextMenuAction::kChangeTone, kChangeToneActionKey},
         {ContextMenuAction::kChangeLength, kChangeLengthActionKey}});

constexpr auto kEntryPointKeys =
    base::MakeFixedFlatMap<EntryPoint, const char*>(
        {{EntryPoint::kOmniboxItem, kOmniboxItemEntryPointKey},
         {EntryPoint::kSidebar, kSidebarEntryPointKey},
         {EntryPoint::kContextMenu, kContextMenuEntryPointKey},
         {EntryPoint::kToolbarButton, kToolbarButtonEntryPointKey},
         {EntryPoint::kMenuItem, kMenuItemEntryPointKey},
         {EntryPoint::kOmniboxCommand, kOmniboxCommandEntryPointKey},
         {EntryPoint::kBraveSearch, kBraveSearchEntryPointKey}});

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

constexpr char kOmniboxInputKey[] = "omnibox_input";
constexpr char kConversationStarterKey[] = "conversation_starter";
constexpr char kPageSummaryKey[] = "page_summary";
constexpr char kTextInputWithPageKey[] = "text_input_with_page";
constexpr char kTextInputWithoutPageKey[] = "text_input_without_page";
constexpr char kTextInputViaFullPageKey[] = "text_input_via_full_page";
constexpr char kQuickActionKey[] = "quick_action";

constexpr auto kContextSourceKeys =
    base::MakeFixedFlatMap<ContextSource, const char*>(
        {{ContextSource::kOmniboxInput, kOmniboxInputKey},
         {ContextSource::kConversationStarter, kConversationStarterKey},
         {ContextSource::kPageSummary, kPageSummaryKey},
         {ContextSource::kTextInputWithPage, kTextInputWithPageKey},
         {ContextSource::kTextInputWithoutPage, kTextInputWithoutPageKey},
         {ContextSource::kTextInputViaFullPage, kTextInputViaFullPageKey},
         {ContextSource::kQuickAction, kQuickActionKey}});

void ReportHistogramForSidebarExperiment(
    int value,
    base::fixed_flat_map<SidebarDefaultMode, const char*, 3> name_map) {
  auto current_mode = sidebar::features::GetSidebarDefaultMode();

  for (int i = 0; i <= static_cast<int>(SidebarDefaultMode::kMaxValue); i++) {
    SidebarDefaultMode i_mode = static_cast<SidebarDefaultMode>(i);
    const char* histogram_name = name_map.at(i_mode);

    // If the mode applies for a given histogram name, report it as usual.
    // If not, do not report & suspend metric, so we don't double count
    // reporting two or more metrics.
    int report_value = current_mode == i_mode ? value : INT_MAX - 1;
    base::UmaHistogramExactLinear(histogram_name, report_value, 3);
  }
}

template <typename EnumType>
uint64_t ReportMostUsedMetric(
    const base::flat_map<EnumType, std::unique_ptr<WeeklyStorage>>& storages,
    const char* histogram_name,
    EnumType max_value) {
  uint64_t total = 0;
  uint64_t total_max = 0;
  std::optional<EnumType> most_used;

  for (int i = 0; i <= static_cast<int>(max_value); i++) {
    EnumType enum_value = static_cast<EnumType>(i);
    uint64_t weekly_total = storages.at(enum_value)->GetWeeklySum();
    if (weekly_total > total_max) {
      most_used = enum_value;
      total_max = weekly_total;
    }
    total += weekly_total;
  }

  if (most_used) {
    UMA_HISTOGRAM_ENUMERATION(histogram_name, *most_used);
  }
  return total;
}

constexpr auto kEnabledHistogramNames =
    base::MakeFixedFlatMap<SidebarDefaultMode, const char*>(
        {{SidebarDefaultMode::kOff, kEnabledHistogramName},
         {SidebarDefaultMode::kAlwaysOn, kEnabledSidebarEnabledAHistogramName},
         {SidebarDefaultMode::kOnOneShot,
          kEnabledSidebarEnabledBHistogramName}});

constexpr auto kUsageWeeklyHistogramNames =
    base::MakeFixedFlatMap<SidebarDefaultMode, const char*>(
        {{SidebarDefaultMode::kOff, kUsageWeeklyHistogramName},
         {SidebarDefaultMode::kAlwaysOn,
          kUsageWeeklySidebarEnabledAHistogramName},
         {SidebarDefaultMode::kOnOneShot,
          kUsageWeeklySidebarEnabledBHistogramName}});

constexpr auto kUsageDailyHistogramNames =
    base::MakeFixedFlatMap<SidebarDefaultMode, const char*>(
        {{SidebarDefaultMode::kOff, kUsageDailyHistogramName},
         {SidebarDefaultMode::kAlwaysOn,
          kUsageDailySidebarEnabledAHistogramName},
         {SidebarDefaultMode::kOnOneShot,
          kUsageDailySidebarEnabledBHistogramName}});

}  // namespace

AIChatMetrics::AIChatMetrics(PrefService* local_state,
                             PrefService* profile_prefs)
    : is_premium_(
          local_state->GetBoolean(prefs::kBraveChatP3ALastPremiumStatus)),
      chat_count_storage_(local_state,
                          prefs::kBraveChatP3AChatCountWeeklyStorage),
      chat_with_history_count_storage_(
          local_state,
          prefs::kBraveChatP3AChatWithHistoryCountWeeklyStorage),
      chat_durations_storage_(local_state,
                              prefs::kBraveChatP3AChatDurationsWeeklyStorage),
      prompt_count_storage_(local_state,
                            prefs::kBraveChatP3APromptCountWeeklyStorage),
      rate_limit_storage_(local_state, prefs::kBraveChatP3ARateLimitStops),
      context_limit_storage_(local_state, prefs::kBraveChatP3AContextLimits),
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
      omnibox_open_storage_(local_state,
                            prefs::kBraveChatP3AOmniboxOpenWeeklyStorage,
                            14),
      omnibox_autocomplete_storage_(
          local_state,
          prefs::kBraveChatP3AOmniboxAutocompleteWeeklyStorage,
          14),
      sidebar_usage_storage_(local_state, prefs::kBraveChatP3ASidebarUsages),
      full_page_switch_storage_(local_state,
                                prefs::kBraveChatP3AFullPageSwitches),
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
      local_state_(local_state) {
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  for (int i = 0; i <= static_cast<int>(ContextMenuAction::kMaxValue); i++) {
    ContextMenuAction action = static_cast<ContextMenuAction>(i);
    context_menu_usage_storages_[action] = std::make_unique<WeeklyStorage>(
        local_state_, prefs::kBraveChatP3AContextMenuUsages,
        kContextMenuActionKeys.at(action));
  }
  for (int i = 0; i <= static_cast<int>(EntryPoint::kMaxValue); i++) {
    EntryPoint entry_point = static_cast<EntryPoint>(i);
    entry_point_storages_[entry_point] = std::make_unique<WeeklyStorage>(
        local_state_, prefs::kBraveChatP3AEntryPointUsages,
        kEntryPointKeys.at(entry_point));
  }
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  for (int i = 0; i <= static_cast<int>(ContextSource::kMaxValue); i++) {
    ContextSource context_source = static_cast<ContextSource>(i);
    context_source_storages_[context_source] = std::make_unique<WeeklyStorage>(
        local_state_, prefs::kBraveChatP3AContextSourceUsages,
        kContextSourceKeys.at(context_source));
  }
  if (profile_prefs) {
    tab_focus_metrics_ = std::make_unique<AIChatTabFocusMetrics>(
        local_state_, profile_prefs, this);
  }
}

AIChatMetrics::~AIChatMetrics() = default;

void AIChatMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kBraveChatP3AChatCountWeeklyStorage);
  registry->RegisterListPref(
      prefs::kBraveChatP3AChatWithHistoryCountWeeklyStorage);
  registry->RegisterListPref(prefs::kBraveChatP3AChatDurationsWeeklyStorage);
  registry->RegisterListPref(prefs::kBraveChatP3APromptCountWeeklyStorage);
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  registry->RegisterListPref(prefs::kBraveChatP3AOmniboxOpenWeeklyStorage);
  registry->RegisterListPref(
      prefs::kBraveChatP3AOmniboxAutocompleteWeeklyStorage);
  registry->RegisterDictionaryPref(prefs::kBraveChatP3AContextMenuUsages);
  registry->RegisterTimePref(prefs::kBraveChatP3ALastContextMenuUsageTime, {});
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  registry->RegisterTimePref(prefs::kBraveChatP3ALastPremiumCheck, {});
  registry->RegisterBooleanPref(prefs::kBraveChatP3ALastPremiumStatus, false);
  registry->RegisterTimePref(prefs::kBraveChatP3AFirstUsageTime, {});
  registry->RegisterTimePref(prefs::kBraveChatP3ALastUsageTime, {});
  registry->RegisterBooleanPref(prefs::kBraveChatP3AUsedSecondDay, false);
  registry->RegisterBooleanPref(prefs::kBraveChatP3AFirstChatPromptsReported,
                                false);
  registry->RegisterDictionaryPref(prefs::kBraveChatP3AContextSourceUsages);
  registry->RegisterDictionaryPref(prefs::kBraveChatP3AEntryPointUsages);
  registry->RegisterListPref(prefs::kBraveChatP3ASidebarUsages);
  registry->RegisterListPref(prefs::kBraveChatP3AFullPageSwitches);
  registry->RegisterListPref(prefs::kBraveChatP3ARateLimitStops);
  registry->RegisterListPref(prefs::kBraveChatP3AContextLimits);
  AIChatTabFocusMetrics::RegisterPrefs(registry);
}

void AIChatMetrics::Bind(mojo::PendingReceiver<mojom::Metrics> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void AIChatMetrics::RecordEnabled(
    bool is_enabled,
    bool is_new_user,
    RetrievePremiumStatusCallback retrieve_premium_status_callback) {
  if (is_enabled && !is_new_user &&
      local_state_->GetTime(prefs::kBraveChatP3AFirstUsageTime).is_null()) {
    // If the user already had AI chat enabled, and we did not record the first
    // & last usage time, set the first & last usage time to a date 90 days ago
    // so we don't skew feature usage metrics.
    base::Time three_months_ago = base::Time::Now() - base::Days(90);
    local_state_->SetTime(prefs::kBraveChatP3AFirstUsageTime, three_months_ago);
    local_state_->SetTime(prefs::kBraveChatP3ALastUsageTime, three_months_ago);
  }

  if (!is_enabled) {
    ReportFeatureUsageMetrics();
    return;
  }

  if (retrieve_premium_status_callback) {
    base::Time last_premium_check =
        local_state_->GetTime(prefs::kBraveChatP3ALastPremiumCheck);
    if (last_premium_check.is_null() ||
        (base::Time::Now() - last_premium_check) >= kPremiumCheckInterval) {
      if (!premium_check_in_progress_) {
        premium_check_in_progress_ = true;
        std::move(retrieve_premium_status_callback)
            .Run(base::BindOnce(&AIChatMetrics::OnPremiumStatusUpdated,
                                weak_ptr_factory_.GetWeakPtr(), is_enabled,
                                is_new_user));
      }
      return;
    }
  }

  is_enabled_ = true;

  ReportHistogramForSidebarExperiment(is_premium_ ? 2 : 1,
                                      kEnabledHistogramNames);
  if (is_new_user && acquisition_source_.has_value()) {
    UMA_HISTOGRAM_ENUMERATION(kAcquisitionSourceHistogramName,
                              *acquisition_source_);
  }

  ReportAllMetrics();
}

void AIChatMetrics::RecordReset() {
  UMA_HISTOGRAM_EXACT_LINEAR(kEnabledHistogramName,
                             std::numeric_limits<int>::max() - 1, 3);
  UMA_HISTOGRAM_EXACT_LINEAR(kAcquisitionSourceHistogramName,
                             std::numeric_limits<int>::max() - 1, 7);
}

void AIChatMetrics::OnPremiumStatusUpdated(bool is_enabled,
                                           bool is_new_user,
                                           mojom::PremiumStatus premium_status,
                                           mojom::PremiumInfoPtr) {
  is_premium_ = premium_status == mojom::PremiumStatus::Active ||
                premium_status == mojom::PremiumStatus::ActiveDisconnected;
  local_state_->SetBoolean(prefs::kBraveChatP3ALastPremiumStatus, is_premium_);
  local_state_->SetTime(prefs::kBraveChatP3ALastPremiumCheck,
                        base::Time::Now());
  premium_check_in_progress_ = false;
  RecordEnabled(is_enabled, is_new_user, {});
}

void AIChatMetrics::RecordNewPrompt(ConversationHandlerForMetrics* handler,
                                    mojom::ConversationPtr& conversation,
                                    mojom::ConversationTurnPtr& entry) {
  const auto& uuid = conversation->uuid;
  if (!conversation_start_times_.contains(uuid)) {
    chat_count_storage_.AddDelta(1);
    if (handler->GetConversationHistorySize() > 1) {
      chat_with_history_count_storage_.AddDelta(1);
    }
    conversation_start_times_[uuid] = base::Time::Now();
  }

  chat_durations_storage_.ReplaceTodaysValueIfGreater(
      (base::Time::Now() - conversation_start_times_[uuid]).InMinutes());

  ReportHistogramForSidebarExperiment(is_premium_ ? 2 : 1,
                                      kUsageDailyHistogramNames);
  ReportHistogramForSidebarExperiment(is_premium_ ? 2 : 1,
                                      kUsageWeeklyHistogramNames);
  UMA_HISTOGRAM_EXACT_LINEAR(kUsageMonthlyHistogramName, is_premium_ ? 2 : 1,
                             3);
  p3a_utils::RecordFeatureUsage(local_state_,
                                prefs::kBraveChatP3AFirstUsageTime,
                                prefs::kBraveChatP3ALastUsageTime);
  ReportFeatureUsageMetrics();
  prompt_count_storage_.AddDelta(1);
  report_debounce_timer_.Start(
      FROM_HERE, kReportDebounceDelay,
      base::BindOnce(&AIChatMetrics::ReportChatCounts, base::Unretained(this)));
  MaybeReportFirstChatPrompts(true);

  RecordContextSource(handler, entry);

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  ReportFullPageUsageMetric();
#endif

  if (handler->should_send_page_contents() &&
      conversation->associated_content &&
      conversation->associated_content->content_used_percentage < 100) {
    context_limit_storage_.AddDelta(1u);
  }
  ReportLimitMetrics();
}

void AIChatMetrics::MaybeRecordLastError(
    ConversationHandlerForMetrics* handler) {
  if (handler->current_error() == mojom::APIError::RateLimitReached &&
      (base::Time::Now() -
       local_state_->GetTime(prefs::kBraveChatP3AFirstUsageTime)) <=
          base::Days(7)) {
    rate_limit_storage_.AddDelta(1u);
    ReportLimitMetrics();
  }
}

void AIChatMetrics::RecordConversationUnload(
    std::string_view conversation_uuid) {
  conversation_start_times_.erase(conversation_uuid);
  MaybeReportFirstChatPrompts(false);
}

void AIChatMetrics::RecordConversationsCleared() {
  conversation_start_times_.clear();
  MaybeReportFirstChatPrompts(false);
}

void AIChatMetrics::OnSendingPromptWithFullPage() {
  prompted_via_full_page_ = true;
}

void AIChatMetrics::OnQuickActionStatusChange(bool is_enabled) {
  prompted_via_quick_action_ = is_enabled;
}

bool AIChatMetrics::IsPremium() const {
  return is_premium_;
}

void AIChatMetrics::MaybeReportFirstChatPrompts(bool new_prompt_made) {
  if (local_state_->GetBoolean(prefs::kBraveChatP3AFirstChatPromptsReported)) {
    return;
  }
  if (new_prompt_made) {
    first_chat_report_debounce_timer_.Start(
        FROM_HERE, kFirstChatPromptsReportDebounceDelay,
        base::BindOnce(&AIChatMetrics::MaybeReportFirstChatPrompts,
                       base::Unretained(this), false));
    return;
  }
  auto prompt_count = prompt_count_storage_.GetWeeklySum();
  if (prompt_count == 0) {
    return;
  }
  p3a_utils::RecordToHistogramBucket(kFirstChatPromptsHistogramName,
                                     kFirstChatPromptsBuckets, prompt_count);
  local_state_->SetBoolean(prefs::kBraveChatP3AFirstChatPromptsReported, true);
}

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
void AIChatMetrics::RecordOmniboxOpen() {
  prompted_via_omnibox_ = true;
  HandleOpenViaEntryPoint(EntryPoint::kOmniboxItem);
  omnibox_open_storage_.AddDelta(1);
  omnibox_autocomplete_storage_.AddDelta(1);
  ReportOmniboxCounts();
}

void AIChatMetrics::RecordOmniboxSearchQuery() {
  omnibox_autocomplete_storage_.AddDelta(1);
  ReportOmniboxCounts();
}

void AIChatMetrics::RecordContextMenuUsage(ContextMenuAction action) {
  HandleOpenViaEntryPoint(EntryPoint::kContextMenu);
  context_menu_usage_storages_[action]->AddDelta(1);
  local_state_->SetTime(prefs::kBraveChatP3ALastContextMenuUsageTime,
                        base::Time::Now());
  ReportContextMenuMetrics();
}

void AIChatMetrics::HandleOpenViaEntryPoint(EntryPoint entry_point) {
  acquisition_source_ = entry_point;

  auto* storage = entry_point_storages_.at(entry_point).get();
  CHECK(storage);
  storage->AddDelta(1u);

  ReportEntryPointUsageMetric();
}

void AIChatMetrics::RecordSidebarUsage() {
  sidebar_usage_storage_.AddDelta(1u);
  ReportFullPageUsageMetric();
}

void AIChatMetrics::RecordFullPageSwitch() {
  full_page_switch_storage_.AddDelta(1u);
  ReportFullPageUsageMetric();
}

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

void AIChatMetrics::ReportAllMetrics() {
  periodic_report_timer_.Start(
      FROM_HERE, base::Time::Now() + kReportInterval,
      base::BindOnce(&AIChatMetrics::ReportAllMetrics, base::Unretained(this)));
  ReportChatCounts();
  ReportFeatureUsageMetrics();
  ReportContextSource();
  ReportLimitMetrics();
  if (tab_focus_metrics_) {
    tab_focus_metrics_->ReportAllMetrics();
  }
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  ReportOmniboxCounts();
  ReportContextMenuMetrics();
  ReportEntryPointUsageMetric();
  ReportFullPageUsageMetric();
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
}

void AIChatMetrics::ReportFeatureUsageMetrics() {
  p3a_utils::RecordFeatureNewUserReturning(
      local_state_, prefs::kBraveChatP3AFirstUsageTime,
      prefs::kBraveChatP3ALastUsageTime, prefs::kBraveChatP3AUsedSecondDay,
      kNewUserReturningHistogramName);
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_state_, prefs::kBraveChatP3ALastUsageTime,
      kLastUsageTimeHistogramName, true);
}

void AIChatMetrics::ReportChatCounts() {
  uint64_t chat_count = chat_count_storage_.GetWeeklySum();

  if (chat_count == 0) {
    // Do not report if AI chat was not used in the past week.
    return;
  }

  uint64_t prompt_count = prompt_count_storage_.GetWeeklySum();
  int average_prompts_per_chat = static_cast<int>(
      std::ceil(static_cast<double>(prompt_count) /
                std::max(chat_count, static_cast<uint64_t>(1))));

  p3a_utils::RecordToHistogramBucket(kChatCountHistogramName, kChatCountBuckets,
                                     chat_count);
  p3a_utils::RecordToHistogramBucket(kAvgPromptCountHistogramName,
                                     kAvgPromptCountBuckets,
                                     average_prompts_per_chat);

  // TODO(djandries): remove the following report when Nebula experiment is over
  p3a_utils::RecordToHistogramBucket(kChatCountNebulaHistogramName,
                                     kChatCountBuckets, chat_count);

  uint64_t max_chat_duration =
      chat_durations_storage_.GetHighestValueInPeriod();
  p3a_utils::RecordToHistogramBucket(kMaxChatDurationHistogramName,
                                     kMaxChatDurationBuckets,
                                     max_chat_duration);

  uint64_t chat_with_history_count =
      chat_with_history_count_storage_.GetWeeklySum();
  int history_percentage = static_cast<int>(std::ceil(
      chat_count > 0
          ? (static_cast<double>(chat_with_history_count) / chat_count) * 100.0
          : 0.0));

  p3a_utils::RecordToHistogramBucket(kChatHistoryUsageHistogramName,
                                     kChatHistoryUsageBuckets,
                                     history_percentage);
}

void AIChatMetrics::RecordContextSource(ConversationHandlerForMetrics* handler,
                                        mojom::ConversationTurnPtr& entry) {
  ContextSource context = ContextSource::kTextInputWithoutPage;
  if (prompted_via_omnibox_) {
    context = ContextSource::kOmniboxInput;
  } else if (entry->action_type == mojom::ActionType::SUMMARIZE_PAGE ||
             entry->action_type == mojom::ActionType::SUMMARIZE_VIDEO) {
    context = ContextSource::kPageSummary;
  } else if (handler->GetConversationHistorySize() == 1 &&
             entry->action_type == mojom::ActionType::CONVERSATION_STARTER) {
    UMA_HISTOGRAM_BOOLEAN(kUsedConversationStarterHistogramName, true);
    context = ContextSource::kConversationStarter;
  } else if (prompted_via_quick_action_) {
    context = ContextSource::kQuickAction;
  } else if (prompted_via_full_page_) {
    context = ContextSource::kTextInputViaFullPage;
  } else if (handler->should_send_page_contents()) {
    context = ContextSource::kTextInputWithPage;
  }
  prompted_via_omnibox_ = false;
  prompted_via_full_page_ = false;
  prompted_via_quick_action_ = false;

  context_source_storages_.at(context)->AddDelta(1u);
  ReportContextSource();
}

void AIChatMetrics::ReportContextSource() {
  if (!is_enabled_) {
    return;
  }
  if (chat_count_storage_.GetWeeklySum() == 0) {
    // Do not report if AI chat was not used in the past week.
    return;
  }

  ReportMostUsedMetric(context_source_storages_,
                       kMostUsedContextSourceHistogramName,
                       ContextSource::kMaxValue);
}

void AIChatMetrics::ReportLimitMetrics() {
  if (!is_enabled_) {
    return;
  }
  if (chat_count_storage_.GetWeeklySum() == 0) {
    // Do not report if AI chat was not used in the past week.
    return;
  }

  p3a_utils::RecordToHistogramBucket(kRateLimitStopsHistogramName,
                                     kRateLimitsBuckets,
                                     rate_limit_storage_.GetWeeklySum());
  p3a_utils::RecordToHistogramBucket(kContextLimitsHistogramName,
                                     kContextLimitsBuckets,
                                     context_limit_storage_.GetWeeklySum());
}

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
void AIChatMetrics::ReportOmniboxCounts() {
  if (!is_enabled_) {
    return;
  }

  base::Time today_midnight = base::Time::Now().LocalMidnight();
  base::Time one_week_ago = today_midnight - base::Days(6);
  base::Time two_weeks_ago = today_midnight - base::Days(13);
  uint64_t autocomplete_count_this_week =
      omnibox_autocomplete_storage_.GetPeriodSumInTimeRange(one_week_ago,
                                                            today_midnight);
  uint64_t autocomplete_count_last_week =
      omnibox_autocomplete_storage_.GetPeriodSumInTimeRange(
          two_weeks_ago, one_week_ago - base::Days(1));
  uint64_t open_count_this_week = omnibox_open_storage_.GetPeriodSumInTimeRange(
      one_week_ago, today_midnight);
  uint64_t open_count_last_week = omnibox_open_storage_.GetPeriodSumInTimeRange(
      two_weeks_ago, one_week_ago - base::Days(1));

  double open_ratio_this_week =
      static_cast<double>(open_count_this_week) /
      std::max(autocomplete_count_this_week, static_cast<uint64_t>(1));
  if (open_ratio_this_week == 0.0) {
    UMA_HISTOGRAM_EXACT_LINEAR(kOmniboxOpensHistogramName, 0, 7);
  } else {
    p3a_utils::RecordToHistogramBucket(
        kOmniboxOpensHistogramName, kOmniboxOpenBuckets,
        static_cast<int>(open_ratio_this_week * 100));
  }

  double open_ratio_last_week =
      static_cast<double>(open_count_last_week) /
      std::max(autocomplete_count_last_week, static_cast<uint64_t>(1));
  if (open_ratio_last_week > 0.0) {
    UMA_HISTOGRAM_BOOLEAN(kOmniboxWeekCompareHistogramName,
                          open_ratio_this_week > open_ratio_last_week);
  }
}

void AIChatMetrics::ReportContextMenuMetrics() {
  if (!is_enabled_) {
    return;
  }

  auto total_usages = ReportMostUsedMetric(
      context_menu_usage_storages_, kMostUsedContextMenuActionHistogramName,
      ContextMenuAction::kMaxValue);

  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_state_, prefs::kBraveChatP3ALastContextMenuUsageTime,
      kContextMenuLastUsageTimeHistogramName, true);

  const char* total_usage_histogram;
  const char* total_usage_histogram_to_remove;
  if (is_premium_) {
    total_usage_histogram = kContextMenuPremiumUsageCountHistogramName;
    total_usage_histogram_to_remove = kContextMenuFreeUsageCountHistogramName;
  } else {
    total_usage_histogram = kContextMenuFreeUsageCountHistogramName;
    total_usage_histogram_to_remove =
        kContextMenuPremiumUsageCountHistogramName;
  }

  p3a_utils::RecordToHistogramBucket(total_usage_histogram,
                                     kContextMenuUsageBuckets, total_usages);
  base::UmaHistogramExactLinear(total_usage_histogram_to_remove, INT_MAX - 1,
                                7);
}

void AIChatMetrics::ReportEntryPointUsageMetric() {
  if (!is_enabled_) {
    return;
  }

  ReportMostUsedMetric(entry_point_storages_, kMostUsedEntryPointHistogramName,
                       EntryPoint::kMaxValue);
}

void AIChatMetrics::ReportFullPageUsageMetric() {
  if (!is_enabled_) {
    return;
  }

  uint64_t sidebar_opens = sidebar_usage_storage_.GetWeeklySum();
  uint64_t full_page_switches = full_page_switch_storage_.GetWeeklySum();

  if (sidebar_opens == 0) {
    return;
  }

  int percentage =
      static_cast<int>(std::ceil((full_page_switches * 100.0) / sidebar_opens));
  p3a_utils::RecordToHistogramBucket(kFullPageSwitchesHistogramName,
                                     kFullPageSwitchesBuckets, percentage);
}

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

}  // namespace ai_chat
