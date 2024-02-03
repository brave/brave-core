/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREF_NAMES_H_

#include "build/build_config.h"

class PrefRegistrySimple;
class PrefService;

namespace ai_chat::prefs {

inline constexpr char kLastAcceptedDisclaimer[] =
    "brave.ai_chat.last_accepted_disclaimer";
inline constexpr char kBraveChatAutocompleteProviderEnabled[] =
    "brave.ai_chat.autocomplete_provider_enabled";
inline constexpr char kBraveChatP3AChatCountWeeklyStorage[] =
    "brave.ai_chat.p3a_chat_count";
inline constexpr char kBraveChatP3APromptCountWeeklyStorage[] =
    "brave.ai_chat.p3a_prompt_count";
// Stores Leo Premium credentials that have already been fetched from the
// SKU SDK but were not used because the chat server was unavailable.
inline constexpr char kBraveChatPremiumCredentialCache[] =
    "brave.ai_chat.premium_credential_cache";
inline constexpr char kUserDismissedPremiumPrompt[] =
    "brave.ai_chat.user_dismissed_premium_prompt";
inline constexpr char kDefaultModelKey[] = "brave.ai_chat.default_model_key";
inline constexpr char kBraveChatP3AOmniboxOpenWeeklyStorage[] =
    "brave.ai_chat.p3a_omnibox_open";
inline constexpr char kBraveChatP3AOmniboxAutocompleteWeeklyStorage[] =
    "brave.ai_chat.p3a_omnibox_autocomplete";
inline constexpr char kBraveChatP3ALastPremiumCheck[] =
    "brave.ai_chat.p3a_last_premium_check";
inline constexpr char kBraveChatP3ALastPremiumStatus[] =
    "brave.ai_chat.p3a_last_premium_status";
inline constexpr char kBraveChatP3AFirstUsageTime[] =
    "brave.ai_chat.p3a_first_usage_time";
inline constexpr char kBraveChatP3ALastUsageTime[] =
    "brave.ai_chat.p3a_last_usage_time";
inline constexpr char kBraveChatP3AUsedSecondDay[] =
    "brave.ai_chat.p3a_used_second_day";
#if BUILDFLAG(IS_ANDROID)
inline constexpr char kBraveChatSubscriptionActiveAndroid[] =
    "brave.ai_chat.subscription_active_android";
inline constexpr char kBraveChatPurchaseTokenAndroid[] =
    "brave.ai_chat.purchase_token_android";
inline constexpr char kBraveChatPackageNameAndroid[] =
    "brave.ai_chat.package_name_android";
inline constexpr char kBraveChatProductIdAndroid[] =
    "brave.ai_chat.product_id_android";
#endif

void RegisterProfilePrefs(PrefRegistrySimple* registry);

void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry);
void MigrateProfilePrefs(PrefService* profile_prefs);

void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

}  // namespace ai_chat::prefs

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREF_NAMES_H_
