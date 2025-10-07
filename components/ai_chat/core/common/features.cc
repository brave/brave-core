/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/features.h"

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "build/build_config.h"

namespace ai_chat::features {

BASE_FEATURE(kAIChat, "AIChat", base::FEATURE_ENABLED_BY_DEFAULT);
const base::FeatureParam<std::string> kAIModelsDefaultKey{
#if BUILDFLAG(IS_IOS)
    &kAIChat, "default_model", "chat-basic"};
#else
    &kAIChat, "default_model", "chat-automatic"};
#endif
const base::FeatureParam<std::string> kAIModelsPremiumDefaultKey{
#if BUILDFLAG(IS_IOS)
    &kAIChat, "default_premium_model", kClaudeSonnetModelKey};
#else
    &kAIChat, "default_premium_model", "chat-automatic"};
#endif
const base::FeatureParam<std::string> kAIModelsVisionDefaultKey{
    &kAIChat, "default_vision_model", kClaudeHaikuModelKey};
const base::FeatureParam<std::string> kAIModelsPremiumVisionDefaultKey{
    &kAIChat, "default_vision_model", kClaudeSonnetModelKey};
const base::FeatureParam<bool> kFreemiumAvailable(&kAIChat,
                                                  "is_freemium_available",
                                                  true);
const base::FeatureParam<bool> kAIChatSSE{&kAIChat, "ai_chat_sse", true};
const base::FeatureParam<bool> kOmniboxOpensFullPage{
    &kAIChat, "omnibox_opens_full_page", true};
const base::FeatureParam<double> kAITemperature{&kAIChat, "temperature", 0.2};

// TODO(petemill): Consider making the algorithm
// more sophisticated and variable by model, event type, or handled by the
// server.
const base::FeatureParam<size_t> kMaxCountLargeToolUseEvents{
    &kAIChat, "max_count_large_tool_use_events", 2};
const base::FeatureParam<size_t> kContentSizeLargeToolUseEvent{
    &kAIChat, "content_size_large_tool_use_events", 1000};

const base::FeatureParam<bool> kShouldIndentPageContentBlocks{
    &kAIChat, "should_indent_page_content_blocks", true};

bool IsAIChatEnabled() {
  return base::FeatureList::IsEnabled(features::kAIChat);
}

BASE_FEATURE(kAIChatHistory,
             "AIChatHistory",
#if BUILDFLAG(IS_IOS)
             base::FEATURE_DISABLED_BY_DEFAULT);
#else
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif

bool IsAIChatHistoryEnabled() {
  return base::FeatureList::IsEnabled(features::kAIChatHistory);
}

BASE_FEATURE(kAIChatFirst, "AIChatFirst", base::FEATURE_DISABLED_BY_DEFAULT);

bool IsAIChatFirstEnabled() {
  return base::FeatureList::IsEnabled(features::kAIChatFirst);
}

BASE_FEATURE(kAIChatTools, "AIChatTools", base::FEATURE_DISABLED_BY_DEFAULT);

bool IsToolsEnabled() {
  return base::FeatureList::IsEnabled(features::kAIChatTools);
}

BASE_FEATURE(kAIChatAgentProfile,
             "AIChatAgentProfile",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsAIChatAgentProfileEnabled() {
#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)
  return base::FeatureList::IsEnabled(features::kAIChatAgentProfile);
#else
  return false;
#endif
}

BASE_FEATURE(kAIChatGlobalSidePanelEverywhere,
             "AIChatGlobalSidePanelEverywhere",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsAIChatGlobalSidePanelEverywhereEnabled() {
  return base::FeatureList::IsEnabled(
      features::kAIChatGlobalSidePanelEverywhere);
}

BASE_FEATURE(kCustomSiteDistillerScripts,
             "CustomSiteDistillerScripts",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsCustomSiteDistillerScriptsEnabled() {
  return base::FeatureList::IsEnabled(features::kCustomSiteDistillerScripts);
}

BASE_FEATURE(kContextMenuRewriteInPlace,
             "AIChatContextMenuRewriteInPlace",
             base::FEATURE_ENABLED_BY_DEFAULT);
bool IsContextMenuRewriteInPlaceEnabled() {
  return base::FeatureList::IsEnabled(features::kContextMenuRewriteInPlace);
}

BASE_FEATURE(kAllowPrivateIPs,
             "AllowPrivateIPs",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsAllowPrivateIPsEnabled() {
  return base::FeatureList::IsEnabled(features::kAllowPrivateIPs);
}

BASE_FEATURE(kOpenAIChatFromBraveSearch,
             "OpenAIChatFromBraveSearch",
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
             base::FEATURE_ENABLED_BY_DEFAULT);
#else
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

bool IsOpenAIChatFromBraveSearchEnabled() {
  return base::FeatureList::IsEnabled(features::kOpenAIChatFromBraveSearch);
}

BASE_FEATURE(kPageContextEnabledInitially,
             "PageContextEnabledInitially",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsPageContextEnabledInitially() {
  return base::FeatureList::IsEnabled(features::kPageContextEnabledInitially);
}

BASE_FEATURE(kTabOrganization,
             "BraveTabOrganization",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsTabOrganizationEnabled() {
  return base::FeatureList::IsEnabled(features::kTabOrganization);
}

}  // namespace ai_chat::features
