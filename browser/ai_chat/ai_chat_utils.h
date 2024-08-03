// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_UTILS_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_UTILS_H_

#include "content/public/browser/browser_context.h"

namespace ai_chat {

bool IsAllowedForContext(content::BrowserContext* context);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_UTILS_H_
