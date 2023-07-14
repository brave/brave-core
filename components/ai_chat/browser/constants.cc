/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/constants.h"

namespace ai_chat {

// Note the blank space intentionally added
constexpr char kHumanPrompt[] = "\n\nHuman: ";
constexpr char kHumanPromptPlaceholder[] = "\nH: ";
constexpr char kAILabelLlama[] = "\n### Response:\n";
constexpr char kAIPrompt[] = "\n\nAssistant:";
constexpr char kAIPromptPlaceholder[] = "\n\nA:";
constexpr char kHumanLabelLlama[] = "\n\n### Instruction:\n";
constexpr char kAIChatCompletionPath[] = "v1/complete";
constexpr char kOpenLlamaModelName[] = "open-llama-13b-open-instruct-8k";

base::span<const webui::LocalizedString> GetLocalizedStrings() {
  constexpr webui::LocalizedString kLocalizedStrings[] = {
      {"siteTitle", IDS_CHAT_UI_TITLE},
      {"summarizeFailedLabel", IDS_CHAT_UI_SUMMARIZE_FAILED_LABEL},
      {"acceptButtonLabel", IDS_CHAT_UI_ACCEPT_BUTTON_LABEL},
      {"summarizeButtonLabel", IDS_CHAT_UI_SUMMARIZE_BUTTON_LABEL},
      {"aboutTitle", IDS_CHAT_UI_ABOUT_TITLE},
      {"aboutDescription", IDS_CHAT_UI_ABOUT_DESCRIPTION},
      {"aboutDescription_2", IDS_CHAT_UI_ABOUT_DESCRIPTION_2},
      {"aboutNote", IDS_CHAT_UI_ABOUT_NOTE},
      {"placeholderLabel", IDS_CHAT_UI_PLACEHOLDER_LABEL},
      {"enableQuestionsTitle", IDS_CHAT_UI_ENABLE_QUESTIONS_TITLE},
      {"enableQuestionsDesc", IDS_CHAT_UI_ENABLE_QUESTIONS_DESC},
      {"enableQuestionsButtonLabel", IDS_CHAT_UI_ENABLE_QUESTIONS_BUTTON_LABEL},
      {"noThanksButtonLabel", IDS_CHAT_UI_NO_THANKS_BUTTON_LABEL}};

  return kLocalizedStrings;
}
}  // namespace ai_chat
