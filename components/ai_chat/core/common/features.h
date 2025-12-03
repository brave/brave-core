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
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<std::string> kAIModelsVisionDefaultKey;
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<std::string> kAIModelsPremiumVisionDefaultKey;

// If true, certain freemium models are available to non-premium users. If
// false, those models are premium-only.
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<bool> kFreemiumAvailable;

COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<bool> kAIChatSSE;
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<bool> kOmniboxOpensFullPage;
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<double> kAITemperature;
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<size_t> kMaxCountLargeToolUseEvents;

// The size of a tool use event's output that triggers that events to
// be marked as "large".
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<size_t> kContentSizeLargeToolUseEvent;

// Whether automatic model should support tools. This affects model routing
// when tools are sent. Since tools are always sent if any are available to the
// conversation and if the model supports them, the server might need to be
// updated to more intelligently ignore tools in certain scenarios.
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<bool> kAutomaticModelSupportsTools;

// Whether should add indentation to page content structure for tool results.
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<bool> kShouldIndentPageContentBlocks;

COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsAIChatEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON) BASE_DECLARE_FEATURE(kAIChatHistory);

COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsAIChatHistoryEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON) BASE_DECLARE_FEATURE(kAIChatFirst);

COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsAIChatFirstEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON) BASE_DECLARE_FEATURE(kAIChatUserChoiceTool);

// Enables experimental features being enabled in a separate profile. If
// disabled, the features will not be enabled anywhere.
COMPONENT_EXPORT(AI_CHAT_COMMON) BASE_DECLARE_FEATURE(kAIChatAgentProfile);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsAIChatAgentProfileEnabled();

// Enables global side panel for any window type (not only agentic)
COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kAIChatGlobalSidePanelEverywhere);
COMPONENT_EXPORT(AI_CHAT_COMMON)
bool IsAIChatGlobalSidePanelEverywhereEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kCustomSiteDistillerScripts);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsCustomSiteDistillerScriptsEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kContextMenuRewriteInPlace);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsContextMenuRewriteInPlaceEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kAllowPrivateIPs);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsAllowPrivateIPsEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kOpenAIChatFromBraveSearch);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsOpenAIChatFromBraveSearchEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kPageContextEnabledInitially);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsPageContextEnabledInitially();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kTabOrganization);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsTabOrganizationEnabled();

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kNEARModels);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsNEARModelsEnabled();

// Whether we should show rich search widgets in the conversation.
COMPONENT_EXPORT(AI_CHAT_COMMON) BASE_DECLARE_FEATURE(kRichSearchWidgets);

// The origin serving the rich search widgets.
// TODO(https://github.com/brave/brave-browser/issues/50901): Remove this once
// we have env setup properly for the origins.
COMPONENT_EXPORT(AI_CHAT_COMMON)
extern const base::FeatureParam<std::string> kRichSearchWidgetsOrigin;

COMPONENT_EXPORT(AI_CHAT_COMMON)
BASE_DECLARE_FEATURE(kCodeExecutionTool);
COMPONENT_EXPORT(AI_CHAT_COMMON) bool IsCodeExecutionToolEnabled();

}  // namespace ai_chat::features

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_FEATURES_H_
