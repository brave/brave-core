/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/constants.h"
#include "base/strings/strcat.h"

namespace ai_chat {

base::span<const webui::LocalizedString> GetLocalizedStrings() {
  constexpr webui::LocalizedString kLocalizedStrings[] = {
      {"siteTitle", IDS_CHAT_UI_TITLE},
      {"summarizeFailedLabel", IDS_CHAT_UI_SUMMARIZE_FAILED_LABEL},
      {"acceptButtonLabel", IDS_CHAT_UI_ACCEPT_BUTTON_LABEL},
      {"summarizeButtonLabel", IDS_CHAT_UI_SUMMARIZE_BUTTON_LABEL},
      {"aboutTitle", IDS_CHAT_UI_ABOUT_TITLE},
      {"aboutDescription", IDS_CHAT_UI_ABOUT_DESCRIPTION},
      {"aboutDescription_2", IDS_CHAT_UI_ABOUT_DESCRIPTION_2},
      {"placeholderLabel", IDS_CHAT_UI_PLACEHOLDER_LABEL},
      {"enableQuestionsTitle", IDS_CHAT_UI_ENABLE_QUESTIONS_TITLE},
      {"enableQuestionsDesc", IDS_CHAT_UI_ENABLE_QUESTIONS_DESC},
      {"enableQuestionsButtonLabel", IDS_CHAT_UI_ENABLE_QUESTIONS_BUTTON_LABEL},
      {"noThanksButtonLabel", IDS_CHAT_UI_NO_THANKS_BUTTON_LABEL},
      {"pageContentWarning", IDS_CHAT_UI_PAGE_CONTENT_WARNING},
      {"errorNetworkLabel", IDS_CHAT_UI_ERROR_NETWORK},
      {"errorRateLimit", IDS_CHAT_UI_ERROR_RATE_LIMIT},
      {"retryButtonLabel", IDS_CHAT_UI_RETRY_BUTTON_LABEL},
      {"introMessage-chat-default", IDS_CHAT_UI_INTRO_MESSAGE_CHAT_DEFAULT},
      {"introMessage-chat-leo-expanded",
       IDS_CHAT_UI_INTRO_MESSAGE_CHAT_LEO_EXPANDED},
      {"introMessage-chat-claude-instant",
       IDS_CHAT_UI_INTRO_MESSAGE_CHAT_LEO_CLAUDE_INSTANT},
      {"modelNameSyntax", IDS_CHAT_UI_MODEL_NAME_SYNTAX},
      {"modelCategory-chat", IDS_CHAT_UI_MODEL_CATEGORY_CHAT},
      {"menuNewChat", IDS_CHAT_UI_MENU_NEW_CHAT},
      {"menuSuggestedQuestions", IDS_CHAT_UI_MENU_SUGGESTED_QUESTIONS},
      {"menuSettings", IDS_CHAT_UI_MENU_SETTINGS},
      {"menuTitleModels", IDS_CHAT_UI_MENU_TITLE_MODELS},
      {"premiumButtonLabel", IDS_CHAT_UI_PREMIUM_BUTTON_LABEL},
      {"rateLimitReachedTitle", IDS_CHAT_UI_RATE_LIMIT_REACHED_TITLE},
      {"rateLimitReachedDesc", IDS_CHAT_UI_RATE_LIMIT_REACHED_DESC},
      {"premiumFeature_1", IDS_CHAT_UI_PREMIUM_FEATURE_1},
      {"premiumFeature_2", IDS_CHAT_UI_PREMIUM_FEATURE_2},
      {"premiumLabel", IDS_CHAT_UI_PREMIUM_LABEL},
      {"premiumPricing", IDS_CHAT_UI_PREMIUM_PRICING},
      {"switchToDefaultModelButtonLabel",
       IDS_CHAT_UI_SWITCH_TO_DEFAULT_MODEL_BUTTON_LABEL},
      {"dismissButtonLabel", IDS_CHAT_UI_DISMISS_BUTTON_LABEL},
      {"unlockPremiumTitle", IDS_CHAT_UI_UNLOCK_PREMIUM_TITLE},
      {"premiumFeature_1_desc", IDS_CHAT_UI_PREMIUM_FEATURE_1_DESC},
      {"premiumFeature_2_desc", IDS_CHAT_UI_PREMIUM_FEATURE_2_DESC},
      {"feedbackSent", IDS_CHAT_UI_FEEDBACK_SENT},
      {"answerDisliked", IDS_CHAT_UI_ANSWER_DISLIKED},
      {"answerLiked", IDS_CHAT_UI_ANSWER_LIKED},
      {"addFeedbackButtonLabel", IDS_CHAT_UI_ADD_FEEDBACK_BUTTON_LABEL},
      {"copyButtonLabel", IDS_CHAT_UI_COPY_BUTTON_LABEL},
      {"likeAnswerButtonLabel", IDS_CHAT_UI_LIKE_ANSWER_BUTTON_LABEL},
      {"dislikeAnswerButtonLabel", IDS_CHAT_UI_DISLIKE_ANSWER_BUTTON_LABEL},
      {"provideFeedbackTitle", IDS_CHAT_UI_PROVIDE_FEEDBACK_TITLE},
      {"selectFeedbackTopic", IDS_CHAT_UI_SELECT_FEEDBACK_TOPIC},
      {"feedbackCategoryLabel", IDS_CHAT_UI_FEEDBACK_CATEGORY_LABEL},
      {"feedbackDescriptionLabel", IDS_CHAT_UI_FEEDBACK_DESCRIPTION_LABEL},
      {"feedbackPremiumNote", IDS_CHAT_UI_FEEDBACK_PREMIUM_NOTE},
      {"submitButtonLabel", IDS_CHAT_UI_SUBMIT_BUTTON_LABEL},
      {"cancelButtonLabel", IDS_CHAT_UI_CANCEL_BUTTON_LABEL},
      {"optionNotHelpful", IDS_CHAT_UI_OPTION_NOT_HELPFUL},
      {"optionIncorrect", IDS_CHAT_UI_OPTION_INCORRECT},
      {"optionUnsafeHarmful", IDS_CHAT_UI_OPTION_UNSAFE_HARMFUL},
      {"optionOther", IDS_CHAT_UI_OPTION_OTHER},
      {"feedbackError", IDS_CHAT_UI_FEEDBACK_SUBMIT_ERROR},
      {"ratingError", IDS_CHAT_UI_RATING_ERROR}};

  return kLocalizedStrings;
}

}  // namespace ai_chat
