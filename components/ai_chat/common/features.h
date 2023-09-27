/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_AI_CHAT_COMMON_FEATURES_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace ai_chat::features {

BASE_DECLARE_FEATURE(kAIChat);
extern const base::FeatureParam<std::string> kAIModelKey;
extern const base::FeatureParam<std::string> kAIModelName;
extern const base::FeatureParam<bool> kAIChatSSE;
extern const base::FeatureParam<double> kAITemperature;

bool IsAIChatEnabled();

BASE_DECLARE_FEATURE(kAIChatHistory);

bool IsAIChatHistoryEnabled();

}  // namespace ai_chat::features

#endif  // BRAVE_COMPONENTS_AI_CHAT_COMMON_FEATURES_H_
