/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/constants.h"

namespace ai_chat {

// Note the blank space intentionally added
constexpr char kHumanPrompt[] = "\n\nHuman: ";
constexpr char kHumanPromptPlaceholder[] = "\n\nH: ";
constexpr char kAIPrompt[] = "\n\nAssistant:";
constexpr char kAIPromptPlaceholder[] = "\n\nA:";
constexpr char kAIChatCompletionPath[] = "v1/complete";

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
      {"loadingLabel", IDS_CHAT_UI_LOADING_LABEL}};

  return kLocalizedStrings;
}
}  // namespace ai_chat
