/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AI_CHAT_PREF_NAMES_H_
#define BRAVE_BROWSER_AI_CHAT_PREF_NAMES_H_

class PrefRegistrySimple;

namespace ai_chat::prefs {

constexpr char kBraveChatHasSeenDisclaimer[] =
    "brave.ai_chat.has_seen_disclaimer";

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace ai_chat::prefs

#endif  // BRAVE_BROWSER_AI_CHAT_PREF_NAMES_H_
