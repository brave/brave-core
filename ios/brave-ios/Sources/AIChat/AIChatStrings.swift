// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public struct AIChat {
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
      value: "Response Rate Limit Reached",
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
    public static let advancedSettingsShowInQSEBarTitle = NSLocalizedString(
      "aichat.advancedSettingsShowInQSEBarTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Show In Quick Search Engines Bar",
      comment: "The title for the settings to show Leo in Quick Search Engines Bar."
    )
    public static let advancedSettingsShowInQSEBarDescription = NSLocalizedString(
      "aichat.advancedSettingsShowInQSEBarDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Enables a shortcut to launch Leo from the quick search engines bar.",
      comment:
        "The description for the settings to show Leo in Quick Search Engines Bar."
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
      value: "View App Store Receipt",
      comment: "The title for the button that allows the user to view the App Store Receipt"
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
    public static let premiumModelStatusTitle = NSLocalizedString(
      "aichat.premiumModelStatusTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Premium",
      comment: "The title of the badge where a model which can be used limited"
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
    public static let leoDisabledPrivateBrowsingMessageTitle = NSLocalizedString(
      "aichat.leoDisabledPrivateBrowsingMessageTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Leo Not Available",
      comment:
        "The title that shows in an alert when the Leo/AI-Chat is disabled due to the user being in private browsing mode."
    )
    public static let leoDisabledPrivateBrowsingMessageDescription = NSLocalizedString(
      "aichat.leoDisabledPrivateBrowsingMessageDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Leo is currently not available in Private Browsing Mode. To use Leo, please exit Private Browsing Mode and try again.",
      comment:
        "The message that shows in an alert, to let the user know the 'Leo' feature is disabled in private browsing mode."
    )
    public static let customModelFormLabelFieldTitle = NSLocalizedString(
      "aichat.customModelFormLabelFieldTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Label",
      comment: "The label for the custom model name field in the custom model form"
    )
    public static let customModelFormLabelFieldPlaceholder = NSLocalizedString(
      "aichat.customModelFormLabelFieldPlaceholder",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Enter a label for this model",
      comment: "The placeholder text for the custom model name field"
    )
    public static let customModelFormRequiredFieldIndicator = NSLocalizedString(
      "aichat.customModelFormRequiredFieldIndicator",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Required",
      comment: "The indicator text shown next to required fields in forms"
    )
    public static let customModelFormLabelFieldFooter = NSLocalizedString(
      "aichat.customModelFormLabelFieldFooter",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Model name to use in Leo's model selector.",
      comment: "The footer text explaining the purpose of the label field"
    )
    public static let customModelFormRequestNameFieldTitle = NSLocalizedString(
      "aichat.customModelFormRequestNameFieldTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Model request name",
      comment: "The label for the model request name field"
    )
    public static let customModelFormRequestNameFieldPlaceholder = NSLocalizedString(
      "aichat.customModelFormRequestNameFieldPlaceholder",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Enter model request name",
      comment: "The placeholder text for the model request name field"
    )
    public static let customModelFormRequestNameFieldFooter = NSLocalizedString(
      "aichat.customModelFormRequestNameFieldFooter",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "The name of the model as it should appear in the request to the serving framework, e.g. phi-3 (note that if this name doesn't match exactly what is expected by the serving framework the integration won't work).",
      comment:
        "The footer text explaining what the model request name is used for and providing an example"
    )
    public static let customModelFormServerEndpointFieldTitle = NSLocalizedString(
      "aichat.customModelFormServerEndpointFieldTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Server endpoint",
      comment: "The label for the server endpoint URL field"
    )
    public static let customModelFormDetailsFieldPlaceholder = NSLocalizedString(
      "aichat.customModelFormDetailsFieldPlaceholder",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Enter details",
      comment: "The placeholder text for generic detail input fields"
    )
    public static let customModelFormServerEndpointFieldFooter = NSLocalizedString(
      "aichat.customModelFormServerEndpointFieldFooter",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "The URL where your serving framework is listening for requests. If you're not sure, check the serving framework documentation. E.g. for Ollama, it is always `http://localhost:11434/v1/chat/completions`. Brave doesn't proxy these requests, please read privacy terms of the chosen provider.",
      comment:
        "The footer text explaining what the server endpoint is and providing an example URL for Ollama"
    )
    public static let customModelFormUnsafeEndpointWarning = NSLocalizedString(
      "aichat.customModelFormUnsafeEndpointWarning",
      tableName: "BraveLeo",
      bundle: .module,
      value: "This endpoint is potentially unsafe.",
      comment: "Warning message shown when user enters an HTTP (not HTTPS) endpoint"
    )
    public static let customModelFormContextSizeFieldTitle = NSLocalizedString(
      "aichat.customModelFormContextSizeFieldTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Context size",
      comment: "The label for the context size field"
    )
    public static let customModelFormContextSizeFieldFooter = NSLocalizedString(
      "aichat.customModelFormContextSizeFieldFooter",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "The maximum number of tokens the model can process in a single interaction. A larger context size allows the model to handle longer conversations. Different models support different maximum context sizes.",
      comment: "The footer text explaining what context size means for AI models"
    )
    public static let customModelFormApiKeyFieldTitle = NSLocalizedString(
      "aichat.customModelFormApiKeyFieldTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "API Key",
      comment: "The label for the API key field"
    )
    public static let customModelFormApiKeyFieldFooter = NSLocalizedString(
      "aichat.customModelFormApiKeyFieldFooter",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Some serving frameworks may require authentication credentials, such as an API key or an access token. These will be added to the request header.",
      comment: "The footer text explaining when and how API keys are used"
    )
    public static let customModelFormVisionSupportFieldTitle = NSLocalizedString(
      "aichat.customModelFormVisionSupportFieldTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Vision support",
      comment: "The label for the vision support toggle"
    )
    public static let customModelFormVisionSupportFieldDescription = NSLocalizedString(
      "aichat.customModelFormVisionSupportFieldDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "If this model takes image as input, e.g. llama 3.2b vision",
      comment: "The description text explaining what vision support means with an example"
    )
    public static let customModelFormSupportsToolsFieldTitle = NSLocalizedString(
      "aichat.customModelFormToolsSupportFieldTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Tools support",
      comment: "The label for the tools support toggle"
    )
    public static let customModelFormSupportsToolsFieldDescription = NSLocalizedString(
      "aichat.customModelFormToolsSupportFieldDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "If this model supports function calling/tool use",
      comment: "The description text explaining what tools supports means"
    )
    public static let customModelFormSystemPromptFieldTitle = NSLocalizedString(
      "aichat.customModelFormSystemPromptFieldTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "System prompt",
      comment: "The label for the system prompt field"
    )
    public static let customModelFormSystemPromptFieldFooter = NSLocalizedString(
      "aichat.customModelFormSystemPromptFieldFooter",
      tableName: "BraveLeo",
      bundle: .module,
      value:
        "Custom instructions and/or context given to the model to guide its responses. Use %datetime% where you would like the current date and time to be inserted.",
      comment:
        "The footer text explaining what a system prompt is and noting the %datetime% placeholder"
    )
    public static let customModelFormAddModelNavigationTitle = NSLocalizedString(
      "aichat.customModelFormAddModelNavigationTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Add Model",
      comment: "The navigation bar title when adding a new custom model"
    )
    public static let customModelFormEditModelNavigationTitle = NSLocalizedString(
      "aichat.customModelFormEditModelNavigationTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Edit Model",
      comment: "The navigation bar title when editing an existing custom model"
    )
    public static let customModelFormSaveButtonTitle = NSLocalizedString(
      "aichat.customModelFormSaveButtonTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Save",
      comment: "The button title for saving a custom model"
    )
    public static let customModelFormDoneButtonTitle = NSLocalizedString(
      "aichat.customModelFormDoneButtonTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Done",
      comment: "The button title for dismissing the keyboard"
    )
    public static let customModelFormFailedToSaveAlertTitle = NSLocalizedString(
      "aichat.customModelFormFailedToSaveAlertTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Failed to Save Model",
      comment: "The alert title shown when saving a custom model fails"
    )
    public static let customModelFormInvalidUrlErrorMessage = NSLocalizedString(
      "aichat.customModelFormInvalidUrlErrorMessage",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Invalid URL",
      comment: "The error message shown when the server endpoint URL is invalid"
    )
    public static let customModelFormInvalidContextSizeErrorMessage = NSLocalizedString(
      "aichat.customModelFormInvalidContextSizeErrorMessage",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Invalid context size",
      comment: "The error message shown when the context size is invalid"
    )
    public static let customModelFormPrivateEndpointErrorMessage = NSLocalizedString(
      "aichat.customModelFormPrivateEndpointErrorMessage",
      tableName: "BraveLeo",
      bundle: .module,
      value: """
        The address you entered appears to be from a private network—like your router or a local server. Brave blocks these automatically to keep you safe, but you can enable them if you're confident it's secure.

        To proceed, visit brave://flags/, search for "brave-ai-chat-allow-private-ips," and enable the feature. Once enabled, you can use private network addresses in Leo settings.
        """,
      comment:
        "The error message shown when trying to use a private network address, explaining why it's blocked and how to enable it"
    )
    public static let byomSectionHeaderTitle = NSLocalizedString(
      "aichat.byomSectionHeaderTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Bring Your Own Model",
      comment: "The section header title for the Bring Your Own Model feature"
    )
    public static let byomSectionHeaderDescription = NSLocalizedString(
      "aichat.byomSectionHeaderDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: """
        Connect your self-hosted or third-party hosted LLM to Leo and use them within the browser. Use any OpenAI Chat Protocol compatible framework such as Ollama. [Learn more](https://support.brave.app/hc/en-us/articles/34070140231821-How-do-I-use-the-Bring-Your-Own-Model-BYOM-with-Brave-Leo)
        """,
      comment:
        "The section header description explaining the Bring Your Own Model feature with a link to learn more. The text inside square brackets should be translated."
    )
    public static let byomEmptyStateTitle = NSLocalizedString(
      "aichat.byomEmptyStateTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "You haven't connected a model yet",
      comment: "The title shown when no custom models have been added yet"
    )
    public static let byomEmptyStateDescription = NSLocalizedString(
      "aichat.byomEmptyStateDescription",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Start by tapping \"Add new model\"",
      comment:
        "The description shown when no custom models have been added, prompting the user to add one"
    )
    public static let byomAddNewModelButtonTitle = NSLocalizedString(
      "aichat.byomAddNewModelButtonTitle",
      tableName: "BraveLeo",
      bundle: .module,
      value: "Add New Model",
      comment: "The button title for adding a new custom model"
    )
  }
}
