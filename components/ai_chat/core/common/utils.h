/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_UTILS_H_

class PrefService;

namespace ai_chat {

bool IsDisabledByPolicy(PrefService* prefs);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_UTILS_H_
