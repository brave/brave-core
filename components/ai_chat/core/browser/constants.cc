/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/constants.h"

#include <utility>

#include "base/strings/strcat.h"
#include "ui/base/l10n/l10n_util.h"

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
      {"aboutDescription_3", IDS_CHAT_UI_ABOUT_DESCRIPTION_3},
      {"placeholderLabel", IDS_CHAT_UI_PLACEHOLDER_LABEL},
      {"pageContentWarning", IDS_CHAT_UI_PAGE_CONTENT_WARNING},
      {"errorNetworkLabel", IDS_CHAT_UI_ERROR_NETWORK},
      {"errorRateLimit", IDS_CHAT_UI_ERROR_RATE_LIMIT},
      {"retryButtonLabel", IDS_CHAT_UI_RETRY_BUTTON_LABEL},
      {"introMessage-chat-basic", IDS_CHAT_UI_INTRO_MESSAGE_CHAT_BASIC},
      {"introMessage-chat-leo-expanded",
       IDS_CHAT_UI_INTRO_MESSAGE_CHAT_LEO_EXPANDED},
      {"introMessage-chat-claude-instant",
       IDS_CHAT_UI_INTRO_MESSAGE_CHAT_LEO_CLAUDE_INSTANT},
      {"introMessage-chat-claude-haiku",
       IDS_CHAT_UI_INTRO_MESSAGE_CHAT_LEO_CLAUDE_HAIKU},
      {"introMessage-chat-claude-sonnet",
       IDS_CHAT_UI_INTRO_MESSAGE_CHAT_LEO_CLAUDE_SONNET},
      {"modelNameSyntax", IDS_CHAT_UI_MODEL_NAME_SYNTAX},
      {"modelPremiumLabelNonPremium",
       IDS_CHAT_UI_MODEL_PREMIUM_LABEL_NON_PREMIUM},
      {"modelCategory-chat", IDS_CHAT_UI_MODEL_CATEGORY_CHAT},
      {"menuNewChat", IDS_CHAT_UI_MENU_NEW_CHAT},
      {"menuGoPremium", IDS_CHAT_UI_MENU_GO_PREMIUM},
      {"menuManageSubscription", IDS_CHAT_UI_MENU_MANAGE_SUBSCRIPTION},
      {"menuSettings", IDS_CHAT_UI_MENU_SETTINGS},
      {"menuTitleModels", IDS_CHAT_UI_MENU_TITLE_MODELS},
      {"suggestQuestionsLabel", IDS_CHAT_UI_SUGGEST_QUESTIONS_LABEL},
      {"upgradeButtonLabel", IDS_CHAT_UI_UPGRADE_BUTTON_LABEL},
      {"rateLimitReachedTitle", IDS_CHAT_UI_RATE_LIMIT_REACHED_TITLE},
      {"rateLimitReachedDesc", IDS_CHAT_UI_RATE_LIMIT_REACHED_DESC},
      {"premiumFeature_1", IDS_CHAT_UI_PREMIUM_FEATURE_1},
      {"premiumFeature_2", IDS_CHAT_UI_PREMIUM_FEATURE_2},
      {"premiumFeature_3", IDS_CHAT_UI_PREMIUM_FEATURE_3},
      {"premiumFeature_4", IDS_CHAT_UI_PREMIUM_FEATURE_4},
      {"premiumFeature_1_desc", IDS_CHAT_UI_PREMIUM_FEATURE_1_DESC},
      {"premiumFeature_2_desc", IDS_CHAT_UI_PREMIUM_FEATURE_2_DESC},
      {"premiumFeature_3_desc", IDS_CHAT_UI_PREMIUM_FEATURE_3_DESC},
      {"premiumFeature_4_desc", IDS_CHAT_UI_PREMIUM_FEATURE_4_DESC},
      {"premiumLabel", IDS_CHAT_UI_PREMIUM_LABEL},
      {"premiumPricing", IDS_CHAT_UI_PREMIUM_PRICING},
      {"switchToBasicModelButtonLabel",
       IDS_CHAT_UI_SWITCH_TO_BASIC_MODEL_BUTTON_LABEL},
      {"dismissButtonLabel", IDS_CHAT_UI_DISMISS_BUTTON_LABEL},
      {"unlockPremiumTitle", IDS_CHAT_UI_UNLOCK_PREMIUM_TITLE},
      {"premiumRefreshWarningDescription",
       IDS_CHAT_UI_PREMIUM_REFRESH_WARNING_DESCRIPTION},
      {"premiumRefreshWarningAction",
       IDS_CHAT_UI_PREMIUM_REFRESH_WARNING_ACTION},
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
      {"saveButtonLabel", IDS_CHAT_UI_SAVE_BUTTON_LABEL},
      {"editedLabel", IDS_CHAT_UI_EDITED_LABEL},
      {"editButtonLabel", IDS_CHAT_UI_EDIT_BUTTON_LABEL},
      {"editAnswerLabel", IDS_CHAT_UI_EDIT_ANSWER_LABEL},
      {"optionNotHelpful", IDS_CHAT_UI_OPTION_NOT_HELPFUL},
      {"optionIncorrect", IDS_CHAT_UI_OPTION_INCORRECT},
      {"optionUnsafeHarmful", IDS_CHAT_UI_OPTION_UNSAFE_HARMFUL},
      {"optionOther", IDS_CHAT_UI_OPTION_OTHER},
      {"feedbackError", IDS_CHAT_UI_FEEDBACK_SUBMIT_ERROR},
      {"ratingError", IDS_CHAT_UI_RATING_ERROR},
      {"braveLeoModelSubtitle-chat-basic", IDS_CHAT_UI_CHAT_BASIC_SUBTITLE},
      {"braveLeoModelSubtitle-chat-leo-expanded",
       IDS_CHAT_UI_CHAT_LEO_EXPANDED_SUBTITLE},
      {"braveLeoModelSubtitle-chat-claude-instant",
       IDS_CHAT_UI_CHAT_CLAUDE_INSTANT_SUBTITLE},
      {"braveLeoModelSubtitle-chat-claude-haiku",
       IDS_CHAT_UI_CHAT_CLAUDE_HAIKU_SUBTITLE},
      {"braveLeoModelSubtitle-chat-claude-sonnet",
       IDS_CHAT_UI_CHAT_CLAUDE_SONNET_SUBTITLE},
      {"clearChatButtonLabel", IDS_CHAT_UI_CLEAR_CHAT_BUTTON_LABEL},
      {"sendChatButtonLabel", IDS_CHAT_UI_SEND_CHAT_BUTTON_LABEL},
      {"errorContextLimitReaching", IDS_CHAT_UI_ERROR_CONTEXT_LIMIT_REACHING},
      {"gotItButtonLabel", IDS_CHAT_UI_GOT_IT_BUTTON_LABEL},
      {"pageContentTooLongWarning", IDS_CHAT_UI_PAGE_CONTENT_TOO_LONG_WARNING},
      {"pageContentRefinedWarning", IDS_CHAT_UI_PAGE_CONTENT_REFINED_WARNING},
      {"pageContentRefinedInProgress",
       IDS_CHAT_UI_PAGE_CONTENT_REFINED_IN_PROGRESS},
      {"errorConversationEnd", IDS_CHAT_UI_CONVERSATION_END_ERROR},
      {"searchInProgress", IDS_CHAT_UI_SEARCH_IN_PROGRESS},
      {"searchQueries", IDS_CHAT_UI_SEARCH_QUERIES},
      {"learnMore", IDS_CHAT_UI_LEARN_MORE},
      {"leoSettingsTooltipLabel", IDS_CHAT_UI_LEO_SETTINGS_TOOLTIP_LABEL},
      {"summarizePageButtonLabel", IDS_CHAT_UI_SUMMARIZE_PAGE},
      {"welcomeGuideTitle", IDS_CHAT_UI_WELCOME_GUIDE_TITLE},
      {"welcomeGuideSubtitle", IDS_CHAT_UI_WELCOME_GUIDE_SUBTITLE},
      {"welcomeGuideSiteHelpCardTitle",
       IDS_CHAT_UI_WELCOME_GUIDE_SITE_HELP_CARD_TITLE},
      {"welcomeGuideSiteHelpCardDesc",
       IDS_CHAT_UI_WELCOME_GUIDE_SITE_HELP_CARD_DESC},
      {"welcomeGuideSiteHelpCardDescWithAction",
       IDS_CHAT_UI_WELCOME_GUIDE_SITE_HELP_CARD_WITH_ACTION},
      {"welcomeGuideShatCardTitle", IDS_CHAT_UI_WELCOME_GUIDE_CHAT_CARD_TITLE},
      {"welcomeGuideShatCardDesc", IDS_CHAT_UI_WELCOME_GUIDE_CHAT_CARD_DESC},
      {"privacyTitle", IDS_CHAT_UI_PRIVACY_TITLE},
      {"contextToggleLabel", IDS_CHAT_UI_CONTEXT_TOGGLE_LABEL},
      {"contextToggleTooltipInfo", IDS_CHAT_UI_CONTEXT_TOGGLE_TOOLTIP_INFO},
      {"subscriptionPolicyInfo", IDS_CHAT_UI_SUBSCRIPTION_POLICY_INFO},
      {"explainLabel", IDS_AI_CHAT_CONTEXT_EXPLAIN},
      {"summarizeLabel", IDS_AI_CHAT_CONTEXT_SUMMARIZE_TEXT},
      {"paraphraseLabel", IDS_AI_CHAT_CONTEXT_PARAPHRASE},
      {"createCategoryTitle", IDS_AI_CHAT_CONTEXT_CREATE},
      {"taglineLabel", IDS_AI_CHAT_CONTEXT_CREATE_TAGLINE},
      {"socialMediaPostLabel", IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_POST},
      {"socialMediaShortLabel",
       IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT},
      {"socialMediaLongLabel",
       IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG},
      {"rewriteCategoryTitle", IDS_AI_CHAT_CONTEXT_REWRITE},
      {"improveLabel", IDS_AI_CHAT_CONTEXT_IMPROVE},
      {"changeToneLabel", IDS_AI_CHAT_CONTEXT_CHANGE_TONE},
      {"changeLengthLabel", IDS_AI_CHAT_CONTEXT_CHANGE_LENGTH},
      {"academicizeLabel", IDS_AI_CHAT_CONTEXT_ACADEMICIZE},
      {"professionalizeLabel", IDS_AI_CHAT_CONTEXT_PROFESSIONALIZE},
      {"persuasiveToneLabel", IDS_AI_CHAT_CONTEXT_PERSUASIVE_TONE},
      {"casualizeLabel", IDS_AI_CHAT_CONTEXT_CASUALIZE},
      {"funnyToneLabel", IDS_AI_CHAT_CONTEXT_FUNNY_TONE},
      {"shortenLabel", IDS_AI_CHAT_CONTEXT_SHORTEN},
      {"expandLabel", IDS_AI_CHAT_CONTEXT_EXPAND},
      {"sendSiteHostnameLabel", IDS_CHAT_UI_SEND_SITE_HOSTNAME_LABEL},
      {"maybeLaterLabel", IDS_AI_CHAT_MAYBE_LATER_LABEL},
      {"toolsMenuButtonLabel", IDS_AI_CHAT_LEO_TOOLS_BUTTON_LABEL},
      {"useMicButtonLabel", IDS_AI_CHAT_USE_MICROPHONE_BUTTON_LABEL},
      {"menuTitleCustomModels", IDS_AI_CHAT_MENU_TITLE_CUSTOM_MODELS},
      {"startConversationLabel", IDS_AI_CHAT_START_CONVERSATION_LABEL}};

  return kLocalizedStrings;
}

std::vector<mojom::ActionGroupPtr> GetActionMenuList() {
  std::vector<mojom::ActionGroupPtr> action_list;

  {
    std::vector<mojom::ActionEntryPtr> entries;
    mojom::ActionGroupPtr group = mojom::ActionGroup::New(
        l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_QUICK_ACTIONS),
        std::move(entries));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_EXPLAIN),
            mojom::ActionType::EXPLAIN)));

    action_list.push_back(std::move(group));
  }

  {
    std::vector<mojom::ActionEntryPtr> entries;
    mojom::ActionGroupPtr group = mojom::ActionGroup::New(
        l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_REWRITE),
        std::move(entries));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_PARAPHRASE),
            mojom::ActionType::PARAPHRASE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_IMPROVE),
            mojom::ActionType::IMPROVE)));

    // Subheading
    auto change_tone_subheading =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_CHANGE_TONE);
    group->entries.push_back(
        mojom::ActionEntry::NewSubheading(change_tone_subheading));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_ACADEMICIZE)},
                " / "),
            mojom::ActionType::ACADEMICIZE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_PROFESSIONALIZE)},
                " / "),
            mojom::ActionType::PROFESSIONALIZE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_PERSUASIVE_TONE)},
                " / "),
            mojom::ActionType::PERSUASIVE_TONE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_CASUALIZE)},
                " / "),
            mojom::ActionType::CASUALIZE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_FUNNY_TONE)},
                " / "),
            mojom::ActionType::FUNNY_TONE)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_SHORTEN)},
                " / "),
            mojom::ActionType::SHORTEN)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {change_tone_subheading,
                 l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_EXPAND)},
                " / "),
            mojom::ActionType::EXPAND)));

    action_list.push_back(std::move(group));
  }

  {
    std::vector<mojom::ActionEntryPtr> entries;
    mojom::ActionGroupPtr group = mojom::ActionGroup::New(
        l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_CREATE),
        std::move(entries));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_CREATE_TAGLINE),
            mojom::ActionType::CREATE_TAGLINE)));

    // Subheading
    auto social_media_subheading =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_POST);
    group->entries.push_back(
        mojom::ActionEntry::NewSubheading(social_media_subheading));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {social_media_subheading,
                 l10n_util::GetStringUTF8(
                     IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT)},
                " / "),
            mojom::ActionType::CREATE_SOCIAL_MEDIA_COMMENT_SHORT)));

    group->entries.push_back(
        mojom::ActionEntry::NewDetails(mojom::ActionDetails::New(
            base::JoinString(
                {social_media_subheading,
                 l10n_util::GetStringUTF8(
                     IDS_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG)},
                " / "),
            mojom::ActionType::CREATE_SOCIAL_MEDIA_COMMENT_LONG)));

    action_list.push_back(std::move(group));
  }

  return action_list;
}

const base::fixed_flat_set<std::string_view, 1> kPrintPreviewRetrievalHosts =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "docs.google.com",
                                             });

constexpr char kLeoModelSupportUrl[] =
    "https://support.brave.com/hc/en-us/categories/"
    "20990938292237-Brave-Leo";
}  // namespace ai_chat
