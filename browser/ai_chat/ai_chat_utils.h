// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_UTILS_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_UTILS_H_

#include "content/public/browser/browser_context.h"

namespace ai_chat {

// Determine whether this BrowserContext is allowed to use AI Chat. When
// |check_policy| is true, it will also check the feature flag and policy.
bool IsAllowedForContext(content::BrowserContext* context,
                         bool check_policy = true);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_UTILS_H_
