/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/ai_chat_utils.h"

#include "base/feature_list.h"
#include "brave/components/ai_chat/features.h"

namespace ai_chat {
bool IsAIChatEnabled() {
  return base::FeatureList::IsEnabled(features::kAIChat);
}

}  // namespace ai_chat
