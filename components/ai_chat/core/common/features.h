/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_FEATURES_H_

#include <string>

#include "base/component_export.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace ai_chat::features {

COMPONENT_EXPORT(AI_CHAT_COMMON) BASE_DECLARE_FEATURE(kAIChat);
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<std::string> kAIModelsDefaultKey;
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<std::string> kAIModelsPremiumDefaultKey;

// If true, certain freemium models are available to non-premium users. If
// false, those models are premium-only.
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<bool> kFreemiumAvailable;

COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<bool> kAIChatSSE;
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<bool> kOmniboxOpensFullPage;
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<bool> kConversationAPIEnabled;
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<double> kAITemperature;

COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsAIChatEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON) BASE_DECLARE_FEATURE(kAIChatHistory);

COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsAIChatHistoryEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kCustomSiteDistillerScripts);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsCustomSiteDistillerScriptsEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kContextMenuRewriteInPlace);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsContextMenuRewriteInPlaceEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON) BASE_DECLARE_FEATURE(kPageContentRefine);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsPageContentRefineEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kAllowPrivateIPs);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsAllowPrivateIPsEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kOpenAIChatFromBraveSearch);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsOpenAIChatFromBraveSearchEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kPageContextEnabledInitially);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsPageContextEnabledInitially();

}  // namespace ai_chat::features

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_FEATURES_H_
