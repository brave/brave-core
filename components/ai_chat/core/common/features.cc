/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/features.h"

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"

namespace ai_chat::features {

BASE_FEATURE(kAIChat,
             "AIChat",
#if BUILDFLAG(ENABLE_AI_CHAT_FEATURE_FLAG)
             base::FEATURE_ENABLED_BY_DEFAULT
#else
             base::FEATURE_DISABLED_BY_DEFAULT
#endif
);
const base::FeatureParam<std::string> kAIModelsDefaultKey{
    &kAIChat, "default_model", "chat-leo-expanded"};
const base::FeatureParam<std::string> kAIModelsPremiumDefaultKey{
    &kAIChat, "default_premium_model", "chat-leo-expanded"};
const base::FeatureParam<bool> kFreemiumAvailable(&kAIChat,
                                                  "is_freemium_available",
                                                  true);
const base::FeatureParam<bool> kAIChatSSE{&kAIChat, "ai_chat_sse", true};
const base::FeatureParam<double> kAITemperature{&kAIChat, "temperature", 0.2};

bool IsAIChatEnabled() {
  return base::FeatureList::IsEnabled(features::kAIChat);
}

BASE_FEATURE(kAIChatHistory,
             "kAIChatHistory",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsAIChatHistoryEnabled() {
  return base::FeatureList::IsEnabled(features::kAIChatHistory);
}

}  // namespace ai_chat::features
