/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_TAB_HELPER_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_TAB_HELPER_FACTORY_H_

#include <memory>

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace ai_chat {

class AIChatTabHelper;

// Returns nullptr when AI Chat is not allowed for the tab's profile.
std::unique_ptr<AIChatTabHelper> MaybeCreateAIChatTabHelper(
    tabs::TabInterface& tab);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_TAB_HELPER_FACTORY_H_
