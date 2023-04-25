/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/features.h"

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace ai_chat::features {

BASE_FEATURE(kAIChat, "AIChat", base::FEATURE_DISABLED_BY_DEFAULT);
const base::FeatureParam<std::string> kAIModelName{&kAIChat, "ai_model_name",
                                                   "claude-v1"};

}  // namespace ai_chat::features
