/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/common/features.h"

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace ai_chat::features {

BASE_FEATURE(kAIChat, "AIChat", base::FEATURE_DISABLED_BY_DEFAULT);
const base::FeatureParam<std::string> kAIModelName{&kAIChat, "ai_model_name",
                                                   "llama-2-13b-chat"};
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
