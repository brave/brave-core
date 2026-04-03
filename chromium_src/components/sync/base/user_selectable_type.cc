/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

constexpr char kAIChatTypeName[] = "aiChat";

#define BRAVE_GET_USER_SELECTABLE_TYPE_INFO \
  case UserSelectableType::kAIChat:         \
    return {kAIChatTypeName, AI_CHAT_CONVERSATION, {AI_CHAT_CONVERSATION}};

#define BRAVE_GET_USER_SELECTABLE_TYPE_FROM_STRING \
  {kAIChatTypeName, UserSelectableType::kAIChat},

#include <components/sync/base/user_selectable_type.cc>

#undef BRAVE_GET_USER_SELECTABLE_TYPE_FROM_STRING
#undef BRAVE_GET_USER_SELECTABLE_TYPE_INFO
