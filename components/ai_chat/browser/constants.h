/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_BROWSER_CONSTANTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_BROWSER_CONSTANTS_H_

#include <string>

#include "components/grit/brave_components_strings.h"
#include "ui/base/webui/web_ui_util.h"

namespace ai_chat {

// Claude
// prefix for for user input
extern const char kHumanPrompt[];
extern const char kHumanPromptPlaceholder[];
// prefix for AI assistant output
extern const char kAIPrompt[];
extern const char kAIPromptPlaceholder[];

// Llama 2
extern const char kLlama2Chat13b[];
extern const char kLlama2Chat13b8k[];
extern const char kLlama2Chat70b[];
extern const char kLlama2Bos[];
extern const char kLlama2Eos[];
extern const char kLlama2BIns[];
extern const char kLlama2EIns[];
extern const char kLlama2BSys[];
extern const char kLlama2ESys[];
extern const char kLlama2DefaultSystemMessage[];

extern const char kAIChatCompletionPath[];

base::span<const webui::LocalizedString> GetLocalizedStrings();

std::string GetHumanPromptSegment();
std::string GetAssistantPromptSegment();

bool UsesLlama2PromptTemplate(const std::string& model);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_BROWSER_CONSTANTS_H_
