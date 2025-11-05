// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_FEATURES_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_FEATURES_H_

#include "base/feature_list.h"

namespace ai_chat::features {

BASE_DECLARE_FEATURE(kAIChatWebUIEnabled);

bool IsAIChatWebUIEnabled();

}  // namespace ai_chat::features

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_FEATURES_H_
