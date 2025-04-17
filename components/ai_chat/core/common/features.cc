/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/features.h"

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "build/build_config.h"

namespace ai_chat::features {

BASE_FEATURE(kAIChat, "AIChat", base::FEATURE_ENABLED_BY_DEFAULT);
const base::FeatureParam<std::string> kAIModelsDefaultKey{
    &kAIChat, "default_model", "chat-leo-expanded"};
const base::FeatureParam<std::string> kAIModelsPremiumDefaultKey{
    &kAIChat, "default_premium_model", "chat-leo-expanded"};
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

BASE_FEATURE(kPageContentRefine,
             "PageContentRefine",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsPageContentRefineEnabled() {
  return base::FeatureList::IsEnabled(features::kPageContentRefine);
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
