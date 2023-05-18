/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONSTANTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONSTANTS_H_

#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace ai_chat {

// prefix for for user input
extern const char kHumanPrompt[];
extern const char kHumanPromptPlaceholder[];

// prefix for AI assistant output
extern const char kAIPrompt[];
extern const char kAIPromptPlaceholder[];

extern const char kAIChatCompletionPath[];

base::span<const webui::LocalizedString> GetLocalizedStrings();

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONSTANTS_H_
