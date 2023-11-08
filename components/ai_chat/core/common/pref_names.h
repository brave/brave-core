/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREF_NAMES_H_

class PrefRegistrySimple;

namespace ai_chat::prefs {

constexpr char kLastAcceptedDisclaimer[] =
    "brave.ai_chat.last_accepted_disclaimer";
constexpr char kBraveChatAutoGenerateQuestions[] =
    "brave.ai_chat.auto_generate_questions";
constexpr char kBraveChatAutocompleteProviderEnabled[] =
    "brave.ai_chat.autocomplete_provider_enabled";
constexpr char kBraveChatP3AChatCountWeeklyStorage[] =
    "brave.ai_chat.p3a_chat_count";
constexpr char kBraveChatP3APromptCountWeeklyStorage[] =
    "brave.ai_chat.p3a_prompt_count";
// Stores Leo Premium credentials that have already been fetched from the
// SKU SDK but were not used because the chat server was unavailable.
constexpr char kBraveChatPremiumCredentialCache[] =
    "brave.ai_chat.premium_credential_cache";
constexpr char kUserDismissedPremiumPrompt[] =
    "brave.ai_chat.user_dismissed_premium_prompt";
constexpr char kDefaultModelKey[] = "brave.ai_chat.default_model_key";
constexpr char kBraveChatP3AOmniboxOpenWeeklyStorage[] =
    "brave.ai_chat.p3a_omnibox_open";
constexpr char kBraveChatP3AOmniboxAutocompleteWeeklyStorage[] =
    "brave.ai_chat.p3a_omnibox_autocomplete";

void RegisterProfilePrefs(PrefRegistrySimple* registry);

void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

}  // namespace ai_chat::prefs

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_PREF_NAMES_H_
