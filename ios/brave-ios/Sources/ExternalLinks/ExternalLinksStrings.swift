// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings

extension Strings {
  public struct ExternalLinks {
    public static let settingTitle = NSLocalizedString(
      "SettingTitle",
      bundle: .module,
      value: "Ask to Open Links from Other Apps",
      comment: "Title of the setting that controls how links opened from other apps are loaded"
    )
    public static let defaultSectionTitle = NSLocalizedString(
      "DefaultSectionTitle",
      bundle: .module,
      value: "Default Behaviour",
      comment: "Section header for the options of how links from other apps are opened"
    )
    public static let currentOption = NSLocalizedString(
      "CurrentOption",
      bundle: .module,
      value: "Current",
      comment: "Option: open links from other apps in the current browsing mode"
    )
    public static let askOption = NSLocalizedString(
      "AskOption",
      bundle: .module,
      value: "Ask",
      comment: "Option: ask how to open each link from other apps"
    )
    public static let regularModeOption = NSLocalizedString(
      "RegularModeOption",
      bundle: .module,
      value: "Regular Mode",
      comment: "Option: always open links from other apps in regular browsing"
    )
    public static let privateModeOption = NSLocalizedString(
      "PrivateModeOption",
      bundle: .module,
      value: "Private Mode",
      comment: "Option: always open links from other apps in private browsing"
    )
    public static let quickViewOption = NSLocalizedString(
      "QuickViewOption",
      bundle: .module,
      value: "QuickView",
      comment: "Option: always open links from other apps in QuickView"
    )
    public static let currentOptionDescription = NSLocalizedString(
      "CurrentOptionDescription",
      bundle: .module,
      value: "Open links in the browsing mode you're currently using, either regular or private.",
      comment:
        "Description of the option to open links from other apps in the current browsing mode"
    )
    public static let askOptionDescription = NSLocalizedString(
      "AskOptionDescription",
      bundle: .module,
      value: "Choose how to open each link. You can remember your choice for a website.",
      comment: "Description of the option to ask how to open each link from other apps"
    )
    public static let regularModeOptionDescription = NSLocalizedString(
      "RegularModeOptionDescription",
      bundle: .module,
      value: "Always open links in a regular tab.",
      comment: "Description of the option to always open links from other apps in regular browsing"
    )
    public static let privateModeOptionDescription = NSLocalizedString(
      "PrivateModeOptionDescription",
      bundle: .module,
      value: "Always open links in a private tab.",
      comment: "Description of the option to always open links from other apps in private browsing"
    )
    public static let quickViewOptionDescription = NSLocalizedString(
      "QuickViewOptionDescription",
      bundle: .module,
      value: "Always open links in QuickView, a preview sheet that doesn't open a new tab.",
      comment: "Description of the option to always open links from other apps in QuickView"
    )
    public static let promptRegularModeDescription = NSLocalizedString(
      "PromptRegularModeDescription",
      bundle: .module,
      value: "Opens this link in a regular tab.",
      comment: "Description in the prompt for opening a link from another app in regular browsing"
    )
    public static let promptPrivateModeDescription = NSLocalizedString(
      "PromptPrivateModeDescription",
      bundle: .module,
      value: "Opens this link in a private tab.",
      comment: "Description in the prompt for opening a link from another app in private browsing"
    )
    public static let promptQuickViewDescription = NSLocalizedString(
      "PromptQuickViewDescription",
      bundle: .module,
      value: "Opens this link in QuickView, a preview sheet that doesn't open a new tab.",
      comment: "Description in the prompt for opening a link from another app in QuickView"
    )
    public static let onboardingTitle = NSLocalizedString(
      "OnboardingTitle",
      bundle: .module,
      value: "Choose How Links Open",
      comment: "Title of the onboarding shown the first time a link is opened from another app"
    )
    public static let onboardingMessage = NSLocalizedString(
      "onboardingMessage",
      bundle: .module,
      value:
        "Brave can ask whether to open links from other apps in a regular tab, private tab or QuickView. You can change this anytime in Settings > Shields & Privacy.",
      comment: "Message of the onboarding shown the first time a link is opened from another app"
    )
    public static let onboardingMessageQuickViewDisabled = NSLocalizedString(
      "onboardingMessageQuickViewDisabled",
      bundle: .module,
      value:
        "Brave can ask whether to open links from other apps in a regular or private tab. You can change this anytime in Settings > Shields & Privacy.",
      comment: "Message of the onboarding shown the first time a link is opened from another app"
    )
    public static let onboardingEnableButtonTitle = NSLocalizedString(
      "OnboardingEnableButtonTitle",
      bundle: .module,
      value: "Ask Me Each Time",
      comment: "Button in the onboarding that turns on asking how to open links from other apps"
    )
    public static let onboardingDeclineButtonTitle = NSLocalizedString(
      "OnboardingDeclineButtonTitle",
      bundle: .module,
      value: "No, Don't Ask Again",
      comment:
        "Button in the onboarding that keeps opening links from other apps in the current browsing mode and doesn't show the onboarding again"
    )
    public static let promptTitle = NSLocalizedString(
      "PromptTitle",
      bundle: .module,
      value: "Open Link",
      comment: "Title of the sheet asking how to open a link from another app"
    )
    public static let rememberChoiceForDomain = NSLocalizedString(
      "RememberChoiceForDomain",
      bundle: .module,
      value: "Remember my choice for %@",
      comment:
        "Toggle to remember the selected way of opening links for a website. %@ is replaced with the website's domain (e.g. 'brave.com')"
    )
    public static let resetRememberedChoices = NSLocalizedString(
      "ResetRememberedChoices",
      bundle: .module,
      value: "Reset",
      comment: "Button that clears all per-site choices for opening links from other apps"
    )
    public static let resetRememberedChoicesFooter = NSLocalizedString(
      "ResetRememberedChoicesFooter",
      bundle: .module,
      value: "Clears the choices remembered for each website when opening links from other apps.",
      comment: "Footer explaining what the reset button does"
    )
    public static let resetRememberedChoicesAlertTitle = NSLocalizedString(
      "ResetRememberedChoicesAlertTitle",
      bundle: .module,
      value: "Reset Remembered Choices?",
      comment:
        "Title of the confirmation alert for clearing per-site choices for opening links from other apps"
    )
    public static let resetRememberedChoicesAlertMessage = NSLocalizedString(
      "ResetRememberedChoicesAlertMessage",
      bundle: .module,
      value: "Brave will ask again how to open links from other apps for every website.",
      comment:
        "Message of the confirmation alert for clearing per-site choices for opening links from other apps"
    )
    public static let resetRememberedChoicesAlertConfirm = NSLocalizedString(
      "ResetRememberedChoicesAlertConfirm",
      bundle: .module,
      value: "Reset",
      comment:
        "Confirm button of the alert for clearing per-site choices for opening links from other apps"
    )
  }
}
