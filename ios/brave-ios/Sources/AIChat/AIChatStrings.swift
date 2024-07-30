// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public struct AIChat {
    public static let contextLimitErrorTitle = NSLocalizedString(
      "aichat.contextLimitErrorTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "This conversation is too long and cannot continue.\nThere may be other models available with which Leo is capable of maintaining accuracy for longer conversations.",
      comment:
        "The title shown on limit reached error view, which is suggesting user to change default model"
    )
    public static let accountSessionExpiredDescription = NSLocalizedString(
      "aichat.accountSessionExpiredDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Your Brave account session has expired. Please visit your account page to refresh, then come back to use premium features.",
      comment:
        "The description for the error message when the user's session has expired"
    )
    public static let newChatActionTitle = NSLocalizedString(
      "aichat.newChatActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "New Chat",
      comment: "The title for button that starts a new chat"
    )
    public static let refreshCredentialsActionTitle = NSLocalizedString(
      "aichat.refreshCredentialsActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Refresh",
      comment: "The title for button that refreshes user credentials"
    )
    public static let networkErrorViewTitle = NSLocalizedString(
      "aichat.networkErrorViewTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "There was a network issue connecting to Leo, check your connection and try again.",
      comment:
        "The title for view that shows network - connection error and suggesting to try again"
    )
    public static let retryActionTitle = NSLocalizedString(
      "aichat.retryActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Retry",
      comment: "The title for button for re-try"
    )
    public static let busyErrorDescription = NSLocalizedString(
      "aichat.busyErrorDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Leo is too busy right now. Please try again in a few minutes.",
      comment:
        "The title for view that shows when leo is too busy (active disconnected) and the user is premium"
    )
    public static let feedbackSuccessAnswerLikedTitle = NSLocalizedString(
      "aichat.feedbackSuccessAnswerLikedTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Answer Liked",
      comment: "The title for feedback view when response is sucessfull also liked"
    )
    public static let feedbackSuccessAnswerDisLikedTitle = NSLocalizedString(
      "aichat.feedbackSuccessAnswerDisLikedTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Answer Disliked",
      comment: "The title for feedback view when response is sucessfull but disliked"
    )
    public static let feedbackSubmittedTitle = NSLocalizedString(
      "aichat.feedbackSubmittedTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Feedback sent successfully",
      comment: "The title for feedback view when it is submitted"
    )
    public static let feedbackSubmittedErrorTitle = NSLocalizedString(
      "aichat.feedbackSubmittedErrorTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Something went wrong. Please try again.",
      comment: "The title for feedback view when it fails to submit feedback"
    )
    public static let addFeedbackActionTitle = NSLocalizedString(
      "aichat.addFeedbackActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Add Feedback",
      comment: "The title for button that submits feedback"
    )
    public static let feedbackOptionTitleNotHelpful = NSLocalizedString(
      "aichat.feedbackOptionTitleNotHelpful",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Not helpful",
      comment: "The title for helpful feedback option"
    )
    public static let feedbackOptionTitleIncorrect = NSLocalizedString(
      "aichat.feedbackOptionTitleIncorrect",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Incorrect",
      comment: "The title for incorrect feedback option"
    )
    public static let feedbackOptionTitleUnsafeHarmful = NSLocalizedString(
      "aichat.feedbackOptionTitleUnsafeHarmful",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Unsafe/harmful",
      comment: "The title for unsafe/harmful feedback option"
    )
    public static let feedbackOptionTitleOther = NSLocalizedString(
      "aichat.feedbackOptionTitleOther",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Other",
      comment: "The title for other feedback option"
    )
    public static let feedbackOptionsViewTitle = NSLocalizedString(
      "aichat.feedbackOptionsViewTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "What's your feedback about?",
      comment: "The title for view which listsfeedback option list"
    )
    public static let feedbackInputViewTitle = NSLocalizedString(
      "aichat.feedbackInputViewTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Provide feedback here",
      comment: "The title for view which user type feedback"
    )
    public static let feedbackViewMainTitle = NSLocalizedString(
      "aichat.feedbackViewMainTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Provide Brave AI Feedback",
      comment: "The title for view which user type feedback"
    )
    public static let feedbackSubmitActionTitle = NSLocalizedString(
      "aichat.feedbackSubmitActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Submit",
      comment: "The title for the button that submits feedback"
    )
    public static let summarizePageActionTitle = NSLocalizedString(
      "aichat.summarizePageActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Summarize this page",
      comment: "The title for button that start summarizing page"
    )
    public static let chatIntroTitle = NSLocalizedString(
      "aichat.chatIntroTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Hi, I'm Leo!",
      comment: "The title for intro view"
    )
    public static let chatIntroSubTitle = NSLocalizedString(
      "aichat.chatIntroSubTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "An AI-powered intelligent assistant, built right into Brave.",
      comment: "The subtitle for intro view"
    )
    public static let chatIntroWebsiteHelpTitle = NSLocalizedString(
      "aichat.chatIntroWebsiteHelpTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Need help with a website?",
      comment: "The title for intro view which triggers website help"
    )
    public static let chatIntroWebsiteHelpSubtitlePageSummarize = NSLocalizedString(
      "aichat.chatIntroWebsiteHelpSubtitlePageSummarize",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "I can help you summarizing articles, expanding on a site's content and much more. Not sure where to start? Try this:",
      comment: "The subtitle for intro view which triggers website help for summary"
    )
    public static let chatIntroWebsiteHelpSubtitleArticleSummarize = NSLocalizedString(
      "aichat.chatIntroWebsiteHelpSubtitleArticleSummarize",
      tableName: "BraveLeo",
      bundle: .module,
      value: "I can help you summarizing articles, expanding on a site's content and much more.",
      comment: "The subtitle for intro view which triggers website help for article"
    )
    public static let chatIntroJustTalkTitle = NSLocalizedString(
      "aichat.chatIntroJustTalkTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Just want to chat?",
      comment: "The title for intro view which triggers just chat"
    )
    public static let chatIntroJustTalkSubTitle = NSLocalizedString(
      "aichat.chatIntroJustTalkSubTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Ask me anything! We can talk about any topic you want. I'm always learning and improving to provide better answers.",
      comment: "The subtitle for intro view which triggers just chat"
    )
    public static let introMessageTitle = NSLocalizedString(
      "aichat.introMessageTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Chat",
      comment: "The title for intro message"
    )
    public static let introMessageLlamaModelDescription = NSLocalizedString(
      "aichat.introMessageLlamaModelDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Llama 3.1 8B by Meta",
      comment:
        "The model and creator for intro message - Llama 3.1 8B is the model name -- Meta is the creator"
    )
    public static let introMessageMixtralModelDescription = NSLocalizedString(
      "aichat.introMessageMixtralModelDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Mixtral by Mistral AI",
      comment:
        "The model and creator for intro message - Mixstral is the model name -- Mistral AI is the creator"
    )
    public static let introMessageClaudeHaikuModelDescription = NSLocalizedString(
      "aichat.introMessageClaudeHaikuModelDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Claude Haiku by Anthropic",
      comment:
        "The model and creator for intro message - Claude Haiku is the model -- Anthropic is the creator"
    )
    public static let introMessageClaudeSonnetModelDescription = NSLocalizedString(
      "aichat.introMessageClaudeSonnetModelDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Claude Sonnet by Anthropic",
      comment:
        "The model and creator for intro message - Claude Sonnet is the model -- Anthropic is the creator"
    )
    public static let introMessageLlamaModelPurposeDescription = NSLocalizedString(
      "aichat.introMessageLlamaModelPurposeDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "General purpose chat",
      comment:
        "The model's purpose - Describes what it can do best"
    )
    public static let introMessageMixtralModelPurposeDescription = NSLocalizedString(
      "aichat.introMessageMixtralModelPurposeDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Advanced chat tasks",
      comment:
        "The model's purpose - Describes what it can do best"
    )
    public static let introMessageClaudeHaikuModelPurposeDescription = NSLocalizedString(
      "aichat.introMessageClaudeHaikuModelPurposeDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Lightning fast chat",
      comment:
        "The model's purpose - Describes what it can do best"
    )
    public static let introMessageClaudeSonnetModelPurposeDescription = NSLocalizedString(
      "aichat.introMessageClaudeSonnetModelPurposeDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Balanced speed and intelligence",
      comment:
        "The model's purpose - Describes what it can do best"
    )
    public static let introMessageLlamaMessageDescription = NSLocalizedString(
      "aichat.introMessageLlamaMessageDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Hi, I'm Leo. I'm a fully hosted AI assistant by Brave. I'm powered by Llama 3.1 8B, a model created by Meta to be performant and applicable to many use cases.",
      comment: "The model intro message when you first enter the chat assistant"
    )
    public static let introMessageMixtralMessageDescription = NSLocalizedString(
      "aichat.introMessageMixtralMessageDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Hi, I'm Leo. I'm a fully hosted AI assistant by Brave. I'm powered by Mixtral 8x7B, a model created by Mistral AI to handle advanced tasks.",
      comment: "The model intro message when you first enter the chat assistant"
    )
    public static let introMessageClaudeHaikuMessageDescription = NSLocalizedString(
      "aichat.introMessageClaudeHaikuMessageDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Hi, I'm Leo. I'm proxied by Brave and powered by Claude 3 Haiku, a model created by Anthropic to power conversational and text processing tasks.",
      comment: "The model intro message when you first enter the chat assistant"
    )
    public static let introMessageClaudeSonnetMessageDescription = NSLocalizedString(
      "aichat.introMessageClaudeSonnetMessageDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Hi, I'm Leo. I'm proxied by Brave and powered by Claude 3.5 Sonnet, a model created by Anthropic to power conversational and text processing tasks.",
      comment: "The model intro message when you first enter the chat assistant"
    )
    public static let introMessageGenericMessageDescription = NSLocalizedString(
      "aichat.introMessageGenericMessageDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Hi, I'm Leo. I'm an AI assistant by Brave. I'm powered by %@. Ask me anything, and I'll do my best to answer.",
      comment:
        "The model intro message when you first enter the chat assistant -- %@ is a place-holder for the model name"
    )
    public static let paywallViewTitle = NSLocalizedString(
      "aichat.paywallViewTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Leo Premium",
      comment: "The title for paywall view"
    )
    public static let restorePaywallButtonTitle = NSLocalizedString(
      "aichat.restorePaywallButtonTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Restore",
      comment: "The button title for restoring ai-app purchse for Leo."
    )
    public static let paywallPurchaseErrorDescription = NSLocalizedString(
      "aichat.paywallPurchaseErrorDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Unable to complete purchase. Please try again, or check your payment details on Apple and try again.",
      comment: "The error description when in app purcahse is erroneous."
    )
    public static let paywallYearlySubscriptionTitle = NSLocalizedString(
      "aichat.paywallYearlySubscriptionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "One Year",
      comment: "Title indicating yearly subscription"
    )
    public static let paywallYearlySubscriptionDescription = NSLocalizedString(
      "aichat.paywallYearlySubscriptionDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "BEST VALUE",
      comment:
        "The description indicating how valuable the yearly subscription is, compared to purchasing monthly"
    )
    public static let paywallYearlyPriceDividend = NSLocalizedString(
      "aichat.paywallYearlyPriceDividend",
      tableName: "BraveLeo",
      bundle: .module,
      value: "year",
      comment: "The text which will be used to indicate period of payments like 150 / year"
    )
    public static let paywallMontlySubscriptionTitle = NSLocalizedString(
      "aichat.paywallMontlySubscriptionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Monthly",
      comment: "Title indicating monthly subscription"
    )
    public static let paywallMonthlyPriceDividend = NSLocalizedString(
      "aichat.paywallMonthlyPriceDividend",
      tableName: "BraveLeo",
      bundle: .module,
      value: "month",
      comment: "The text which will be used to indicate period of payments like 10 / month"
    )
    public static let paywallPurchaseDeepNote = NSLocalizedString(
      "aichat.paywallPurchaseDeepNote",
      tableName: "BraveLeo",
      bundle: .module,
      value: "All subscriptions are auto-renewed but can be cancelled at any time before renewal.",
      comment:
        "The text displayed on the bottom of paywall screen which indicates subscriptions are auto renewable."
    )
    public static let paywallPurchaseActionTitle = NSLocalizedString(
      "aichat.paywallPurchaseActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Upgrade Now",
      comment: "The title of the button for action triggering purchase"
    )
    public static let paywallPurchaseActionIntroOfferTitle = NSLocalizedString(
      "aichat.paywallPurchaseActionIntroOfferTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Try 7 Days Free",
      comment: "The title of the button for action triggering purchase"
    )
    public static let paywallPremiumUpsellTitle = NSLocalizedString(
      "aichat.paywallPremiumUpsellTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Unleash Leo's Full Powers With Premium:",
      comment: "The title for premium upsell when paywall is triggered"
    )
    public static let paywallRateLimitTitle = NSLocalizedString(
      "aichat.paywallRateLimitTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Unleash Leo's Full Powers With Premium:",
      comment: "The title for premium upseel when rate limit is shown after sending a message"
    )
    public static let paywallRateLimitSubTitle = NSLocalizedString(
      "aichat.paywallRateLimitSubTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Unlock a higher response rate by subscribing to Premium, or try again later.",
      comment: "The subtitle for premium upseel when rate limit is shown after sending a message"
    )
    public static let paywallPremiumUpsellPrimaryAction = NSLocalizedString(
      "aichat.paywallPremiumUpsellPrimaryAction",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Upgrade",
      comment: "The title for button when premium upsell when paywall is triggered"
    )
    public static let paywallPremiumUpsellDismissAction = NSLocalizedString(
      "aichat.paywallPremiumUpsellDismissAction",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Maybe Later",
      comment: "The title for dismiss button when premium upsell when paywall is triggered"
    )
    public static let paywallUpsellModelTypeTopicTitle = NSLocalizedString(
      "aichat.paywallUpsellModelTypeTopicTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Explore different AI models",
      comment: "The title model type entry in paywall upsell screen"
    )
    public static let paywallUpsellCreativityTopicTitle = NSLocalizedString(
      "aichat.paywallUpsellCreativityTopicTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Unlock your creativity",
      comment: "The title creativity entry in paywall upsell screen"
    )
    public static let paywallUpsellAccuracyTopicTitle = NSLocalizedString(
      "aichat.paywallUpsellAccuracyTopicTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Stay on topic",
      comment: "The title accuracy entry in paywall upsell screen"
    )
    public static let paywallUpsellChatLengthTopicTitle = NSLocalizedString(
      "aichat.paywallUpsellChatLengthTopicTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Chat for longer",
      comment: "The title chat length entry in paywall upsell screen"
    )
    public static let paywallUpsellModelTypeTopicSubTitle = NSLocalizedString(
      "aichat.paywallUpsellModelTypeTopicSubTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Priority access to powerful models with different skills.",
      comment: "The subtitle model type entry in paywall upsell screen"
    )
    public static let paywallUpsellCreativityTopicSubTitle = NSLocalizedString(
      "aichat.paywallUpsellCreativityTopicSubTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Access models better suited for creative tasks and content generation.",
      comment: "The subtitle creativity entry in paywall upsell screen"
    )
    public static let paywallUpsellAccuracyTopicSubTitle = NSLocalizedString(
      "aichat.paywallUpsellAccuracyTopicSubTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Get more accurate answers for more nuanced conversations.",
      comment: "The subtitle accuracy entry in paywall upsell screen"
    )
    public static let paywallUpsellChatLengthTopicSubTitle = NSLocalizedString(
      "aichat.paywallUpsellChatLengthTopicSubTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Get higher rate limits for longer conversations.",
      comment: "The subtitle chat length entry in paywall upsell screen"
    )
    public static let leoNavigationTitle = NSLocalizedString(
      "aichat.leoNavigationTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Leo AI",
      comment: "The title of the advanced settings view"
    )
    public static let manageSubscriptionsButtonTitle = NSLocalizedString(
      "aichat.manageSubscriptionsButtonTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Manage Subscriptions",
      comment: "The button title for managing subscriptions"
    )
    public static let goPremiumButtonTitle = NSLocalizedString(
      "aichat.goPremiumButtonTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Go Premium",
      comment: "The button title for opening paywall screen"
    )
    public static let monthlySubscriptionTitle = NSLocalizedString(
      "aichat.monthlySubscriptionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Monthly Subscription",
      comment: "Title showing monthly subscription"
    )
    public static let yearlySubscriptionTitle = NSLocalizedString(
      "aichat.yearlySubscriptionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Yearly Subscription",
      comment: "Title showing yearly subscription - annual"
    )
    public static let premiumSubscriptionTitle = NSLocalizedString(
      "aichat.premiumSubscriptionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Premium Subscription",
      comment: "Title showing premium subscription - not determined monthly por yearly"
    )
    public static let subscriptionNoneTitle = NSLocalizedString(
      "aichat.subscriptionNoneTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "None",
      comment: "Title showing the user has no subscription."
    )
    public static let advancedSettingsAutocompleteTitle = NSLocalizedString(
      "aichat.advancedSettingsAutocompleteTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Show In Address Bar",
      comment: "The title for the settings to show search suggestions from Leo in url-address bar"
    )
    public static let advancedSettingsAutocompleteDescription = NSLocalizedString(
      "aichat.advancedSettingsAutocompleteDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Turn on to show a shortcut to launch Leo from the address bar.",
      comment:
        "The description for the settings to show search suggestions from Leo in url-address bar"
    )
    public static let advancedSettingsDefaultModelTitle = NSLocalizedString(
      "aichat.advancedSettingsDefaultModelTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Default Model",
      comment: "The title for the settings to change default model for conversations"
    )
    public static let advancedSettingsHeaderTitle = NSLocalizedString(
      "aichat.advancedSettingsHeaderTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Leo is an AI-powered smart assistant, built right into the browser.",
      comment: "The title for the header for adjusting leo ai settings"
    )
    public static let advancedSettingsSubscriptionStatusTitle = NSLocalizedString(
      "aichat.advancedSettingsSubscriptionStatusTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Status",
      comment: "The title for the label showing status if the subscription"
    )
    public static let advancedSettingsSubscriptionExpiresTitle = NSLocalizedString(
      "aichat.advancedSettingsSubscriptionExpiresTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Expires",
      comment: "The title for the label showing the date which subscription expires"
    )
    public static let advancedSettingsLinkPurchaseActionTitle = NSLocalizedString(
      "aichat.advancedSettingsLinkPurchaseActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Link purchase to your Brave account",
      comment: "The title for the button which links purchase to Brave Account"
    )
    public static let advancedSettingsLinkPurchaseActionSubTitle = NSLocalizedString(
      "aichat.advancedSettingsLinkPurchaseActionSubTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Link your App Store purchase to your Brave account to use Leo on other devices.",
      comment: "The subtitle for the button which links purchase to Brave Account"
    )
    public static let advancedSettingsSubscriptionHeaderTitle = NSLocalizedString(
      "aichat.advancedSettingsSubscriptionHeaderTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Subscription",
      comment: "The title for the header for subscription details"
    )
    public static let advancedSettingsViewReceiptTitle = NSLocalizedString(
      "aichat.advancedSettingsViewReceiptTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "View AppStore Receipt",
      comment: "The title for the button that allows the user to view the AppStore Receipt"
    )
    public static let appStoreErrorTitle = NSLocalizedString(
      "aichat.appStoreErrorTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "App Store Error",
      comment: "The title for the error showing there is an error from App Store"
    )
    public static let appStoreErrorSubTitle = NSLocalizedString(
      "aichat.appStoreErrorSubTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Could not connect to App Store, please try again later.",
      comment: "The subtitle for the error showing there is an error from App Store"
    )
    public static let resetLeoDataActionTitle = NSLocalizedString(
      "aichat.resetLeoDataActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Reset And Clear Leo Data",
      comment: "The title for the button where it triggers reset leo data"
    )
    public static let resetLeoDataErrorTitle = NSLocalizedString(
      "aichat.resetLeoDataErrorTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Reset Leo Data",
      comment: "The title for the error whre leo data reset"
    )
    public static let resetLeoDataErrorDescription = NSLocalizedString(
      "aichat.resetLeoDataErrorDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Are you sure you want to reset and clear Leo Data?",
      comment: "The description for the error whre leo data reset"
    )
    public static let resetLeoDataAlertButtonTitle = NSLocalizedString(
      "aichat.resetLeoDataAlertButtonTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Reset",
      comment: "The title of a button that will reset data for leo"
    )
    public static let defaultModelViewTitle = NSLocalizedString(
      "aichat.defaultModelViewTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Default Model",
      comment: "The title of the menu where user can change the default model"
    )
    public static let defaultModelChatSectionTitle = NSLocalizedString(
      "aichat.defaultModelChatSectionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Chat",
      comment: "The title of the section where chat models are displayed as a list."
    )
    public static let customModelChatSectionTitle = NSLocalizedString(
      "aichat.customModelChatSectionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Custom Models",
      comment: "The title of the section where custom models are displayed as a list."
    )
    public static let unlimitedModelStatusTitle = NSLocalizedString(
      "aichat.unlimitedModelStatusTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Unlimited",
      comment: "The title of the badge where a model which can be used unlimited"
    )
    public static let premiumModelStatusTitle = NSLocalizedString(
      "aichat.premiumModelStatusTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Premium",
      comment: "The title of the badge where a model which can be used limited"
    )
    public static let defaultModelLanguageSectionTitle = NSLocalizedString(
      "aichat.defaultModelLanguageSectionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "LANGUAGE MODELS",
      comment: "The title of the section where language models are displayed as a list."
    )
    public static let quickMenuGoPremiumActionTitle = NSLocalizedString(
      "aichat.quickMenuGoPremiumActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Go Premium",
      comment: "The title of action for quick menu which presents payment screen for premium"
    )
    public static let quickMenuManageSubscriptionActionTitle = NSLocalizedString(
      "aichat.quickMenuManageSubscriptionActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Manage Subscriptions",
      comment: "The title of action for quick menu which display subscription management view"
    )
    public static let quickMenuAdvancedSettingsActionTitle = NSLocalizedString(
      "aichat.quickMenuAdvancedSettingsActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Advanced Settings",
      comment: "The title of action for quick menu which displaying advanced settings"
    )
    public static let premiumNavigationBarBadgeTitle = NSLocalizedString(
      "aichat.premiumNavigationBarBadgeTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Premium",
      comment: "Title shown next to Leo in navigation bar when user has premium subcription"
    )
    public static let infoAboutPageContext = NSLocalizedString(
      "aichat.infoAboutPageContext",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Shape answers based on the page's contents",
      comment: "Description about answers are changing with page context"
    )
    public static let promptPlaceHolderDescription = NSLocalizedString(
      "aichat.promptPlaceHolderDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Enter a prompt here",
      comment: "The text for placeholder on textfield for entering questions for AI"
    )
    public static let termsConditionsTitle = NSLocalizedString(
      "aichat.termsConditionsTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Chat Privately with Brave Leo",
      comment: "The title terms and conditions for AI chat usage"
    )
    public static let termsConditionsDescription = NSLocalizedString(
      "aichat.termsConditionsDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Brave Leo is a private AI smart assistant that enhances your use of the web. Leo is free to use with limited access. Brave Leo Premium offers more models, higher limits and gives subscribers early access to new features. The default model for all users is currently Mixtral 8x7B. See the Brave **[Help Center](%@)** for more details.\n\nBy default, when you ask Leo a question it may send and use the content of the web page you are viewing or any text you highlight on a page to Brave Leo to provide a response. In some cases, Leo will send queries derived from your input to Brave Search and use the search results to better answer your question. The accuracy of responses is not guaranteed, and may include inaccurate, misleading, or false information. Don't submit sensitive or private info, and use caution with any answers related to health, finance, personal safety, or similar. You can adjust Leo’s options in Settings any time. Go to Settings > Leo.\n\nLeo does not collect identifiers such as your IP address that can be linked to you. No data is used to train the AI models and no personal data is retained by the AI models (except for Anthropic that holds data for 30 days before deleting it). See the **[privacy policy](%@)** for more information.",
      comment:
        "The description terms and conditions for AI chat usage. The links are inside parenthesis %@ will be replaced with urls"
    )
    public static let termsConditionsApprovalActionTitle = NSLocalizedString(
      "aichat.termsConditionsApproveActionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "I Understand",
      comment: "The title for expressing acknowledgement for the terms and conditions of Brave AI"
    )
    public static let feedbackPremiumAdTitle = NSLocalizedString(
      "aichat.feedbackPremiumAdTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Leo Premium provides access to an expanded set of language models for even greater answer nuance. [Learn more](https://brave.com/) - [Dismiss](braveai://dismiss)",
      comment:
        "The title for premium ad view. The links are inside parenthesis and the phrases inside square brackets should be translated"
    )
    public static let speechRecognizerDisclaimer = NSLocalizedString(
      "aichat.speechRecognizerDisclaimer",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Brave does not store or share your voice inputs.",
      comment:
        "The description indicating voice inputs from user is not store or shared and these inpurs are totally private."
    )
    public static let microphonePermissionAlertTitle = NSLocalizedString(
      "aichat.microphonePermissionAlertTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Microphone Access Required",
      comment: "The description for alert needed to have microphone permission"
    )
    public static let microphonePermissionAlertDescription = NSLocalizedString(
      "aichat.microphonePermissionAlertDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Please allow microphone access in iOS system settings for Brave to use anonymous voice entry.",
      comment: "The description for alert needed to have microphone permission"
    )
    public static let voiceInputButtonTitle = NSLocalizedString(
      "aichat.voiceInputButtonTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Voice Input",
      comment:
        "The accessibility description when pressing the microphone button to access voice entry."
    )
    public static let responseContextMenuEditPromptTitle = NSLocalizedString(
      "aichat.responseContextMenuEditPromptTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Edit Prompt",
      comment: "The title for editing user prompt action from context menu"
    )
    public static let responseContextMenuEditAnswerTitle = NSLocalizedString(
      "aichat.responseContextMenuEditAnswerTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Edit Answer",
      comment: "The title for editing AI response action from context menu"
    )
    public static let responseContextMenuRegenerateTitle = NSLocalizedString(
      "aichat.responseContextMenuRegenerateTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Regenerate",
      comment: "The title for regenerating response action from context menu"
    )
    public static let responseContextMenuCopyTitle = NSLocalizedString(
      "aichat.responseContextMenuCopyTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Copy",
      comment: "The title for copying response to clipboard action "
    )
    public static let responseContextMenuLikeAnswerTitle = NSLocalizedString(
      "aichat.responseContextMenuLikeAnswerTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Like Answer",
      comment: "The title for liking response to clipboard action"
    )
    public static let responseContextMenuDislikeAnswerTitle = NSLocalizedString(
      "aichat.responseContextMenuDislikeAnswerTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Dislike Answer",
      comment: "The title for disliking response to clipboard action"
    )
    public static let rateAnswerActionErrorText = NSLocalizedString(
      "aichat.rateAnswerActionErrorText",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Error Rating Answer",
      comment: "The title for error when rating a response is not success"
    )
    public static let suggestionsGenerationButtonTitle = NSLocalizedString(
      "aichat.suggestionsGenerationButtonTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Suggest questions...",
      comment: "The title for the button to generate suggestions"
    )
    public static let chatMenuSectionTitle = NSLocalizedString(
      "aichat.chatMenuSectionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Chat",
      comment: "The title for the chat section in the menu"
    )
    public static let customModelsMenuSectionTitle = NSLocalizedString(
      "aichat.customModelsMenuSectionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Custom Models",
      comment: "The title for the custom models section in the menu"
    )
    public static let askLeoSearchSuggestionTitle = NSLocalizedString(
      "aichat.askLeoSearchSuggestionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Ask Leo",
      comment: "The title of the search bar suggestion when prompting to Ask Leo"
    )
    public static let leoSubscriptionUnknownDateTitle = NSLocalizedString(
      "aichat.leoSubscriptionUnknownDateTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "N/A",
      comment: "The title to display when the duration of the subscription is unknown"
    )
    public static let leoPageContextInfoIconAccessibilityTitle = NSLocalizedString(
      "aichat.leoPageContextInfoIconAccessibilityTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Page Context Information",
      comment:
        "The accessibility text to read out loud when the user taps on the info icon on the page context view"
    )
    public static let leoPageContextInfoDescriptionTitle = NSLocalizedString(
      "aichat.leoPageContextInfoDescriptionTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Toggle on to ask about this page. Its content will be sent to Brave Leo along with your messages.",
      comment:
        "The title to display when the user taps on the information icon on the page context view. Brave Leo is the name of the product and should not be translated."
    )
    public static let leoSlashToolsButtonAccessibilityTitle = NSLocalizedString(
      "aichat.leoSlashToolsButtonAccessibilityTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Slash Tools",
      comment:
        "The text that's read aloud to the user when accessibility is enabled for the Slash Tools button"
    )
    public static let leoSlashToolsOptionNull = NSLocalizedString(
      "aichat.leoSlashToolsOptionNull",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "None",
      comment:
        "The text that's displayed when the user selects an invalid slash tools option"
    )
    public static let leoSlashToolsOptionUnknown = NSLocalizedString(
      "aichat.leoSlashToolsOptionUnknown",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Unknown",
      comment:
        "The text that's displayed when the user selects an invalid slash tools option"
    )
    public static let leoImprovingAnswerBraveSearch = NSLocalizedString(
      "aichat.leoImprovingAnswerBraveSearch",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Improving answer with Brave Search...",
      comment:
        "The text displayed on the loading screen when searching for a user query"
    )
    public static let leoPageContentRefineInProgress = NSLocalizedString(
      "aichat.leoPageContentRefineInProgress",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Finding the relevant parts of the web page...",
      comment:
        "An indicator to show page content refined is in progress"
    )
    public static let leoImprovedAnswerBraveSearch = NSLocalizedString(
      "aichat.leoImprovedAnswerBraveSearch",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Improved answer searching for",
      comment:
        "The text displayed on the loading screen when searching for a user query"
    )
    public static let youMessageTitle = NSLocalizedString(
      "aichat.youMessageTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "You",
      comment:
        "The word 'You' as in 'Me', 'Myself', 'I' (the user)."
    )
    public static let leoAssistantNameTitle = NSLocalizedString(
      "aichat.leoAssistantNameTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Leo",
      comment:
        "The name of the AI-Assistant that's responsing to the user"
    )
    public static let editedMessageCaption = NSLocalizedString(
      "aichat.editedMessageCaption",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Edited",
      comment: "The text displayed under an edited user prompt/message beside a timestamp."
    )
    public static let leoCodeExampleDefaultTitle = NSLocalizedString(
      "aichat.leoCodeExampleDefaultTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Code example",
      comment:
        "The default title displayed above a code block when the language of the code is not known."
    )
    public static let leoDisabledMessageTitle = NSLocalizedString(
      "aichat.leoDisabledMessageTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Leo Disabled",
      comment:
        "The title that shows in an alert when the Leo/AI-Chat feature is disabled."
    )
    public static let leoDisabledMessageDescription = NSLocalizedString(
      "aichat.leoDisabledMessageDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Leo is currently disabled via feature flags. To re-enable Leo, please visit brave://flags and enable it.",
      comment:
        "The message that shows in an alert, to let the user know the 'Leo' feature is disabled, and explains how to re-enable the feature."
    )
  }
}
