// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public struct FocusOnboarding {
    public static let continueButtonTitle = NSLocalizedString(
      "focusOnboarding.continueButtonTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Continue",
      comment: "The title of the button that changes the screen to next"
    )

    public static let movingAdsScreenTitle = NSLocalizedString(
      "focusOnboarding.movingAdsScreenTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Fewer ads & trackers.",
      comment: "The title of the screen that shows ads are blocked"
    )

    public static let movingAdsScreenDescription = NSLocalizedString(
      "focusOnboarding.movingAdsScreenDescription",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Browse faster and use less data.",
      comment: "The subtitle of the screen that shows ads are blocked"
    )

    public static let noVideoAdsScreenTitle = NSLocalizedString(
      "focusOnboarding.noVideoAdsScreenTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "No more video ads.",
      comment: "The title of the screen that shows ads embeded in video are blocked"
    )

    public static let noVideoAdsScreenDescription = NSLocalizedString(
      "focusOnboarding.noVideoAdsScreenDescription",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Seriously, we got rid of them.",
      comment: "The subtitle of the screen that shows ads embeded in video are blocked"
    )

    public static let p3aScreenTitle = NSLocalizedString(
      "focusOnboarding.p3aScreenTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Make Brave Better.",
      comment: "The title of the screen which asks user to enable privacy preserving analytics."
    )

    public static let p3aScreenDescription = NSLocalizedString(
      "focusOnboarding.p3aScreenDescription",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Let us know which features you’re enjoying the most.",
      comment: "The subtitle of the screen that asks user to enable privacy preserving analytics."
    )

    public static let p3aToggleTitle = NSLocalizedString(
      "focusOnboarding.p3aToggleTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Share Private & Anonymous Product Insights.",
      comment: "The title of the toggle for enable / disable the privacy preserving analytics."
    )

    public static let p3aToggleDescription = NSLocalizedString(
      "focusOnboarding.p3aToggleDescription",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Change this at any time in Brave Settings under ‘Brave Shields and Privacy’.",
      comment: "The title of the toggle for enable / disable privacy preserving analytics."
    )

    public static let p3aInformationButtonTitle = NSLocalizedString(
      "focusOnboarding.p3aInformationButtonTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Learn more about our Privacy Preserving Product Analytics (P3A)",
      comment: "The title of the button that opens the website abouyt privacy preserving analytics."
    )

    public static let defaultBrowserScreenTitle = NSLocalizedString(
      "focusOnboarding.defaultBrowserScreenTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Set Brave as your Default Browser",
      comment: "The title of the screen that requests user to set Brave as default"
    )

    public static let defaultBrowserScreenDescription = NSLocalizedString(
      "focusOnboarding.defaultBrowserScreenDescription",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Open every link you tap with Brave’s privacy protections",
      comment: "The subtitle of the screen that requests user to set Brave as default"
    )

    public static let systemSettingsButtonTitle = NSLocalizedString(
      "focusOnboarding.systemSettingsButtonTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Go To System Settings",
      comment: "The title of the button that triggers navigation link to settings"
    )

    public static let startBrowseActionButtonTitle = NSLocalizedString(
      "focusOnboarding.startBrowseActionButtonTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Start Browsing",
      comment: "The title of the button that finishes the onboarding without setting default"
    )

    public static let notNowActionButtonTitle = NSLocalizedString(
      "focusOnboarding.notNowActionButtonTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Not Now",
      comment: "The title of the button that closes the default browser full screen callout"
    )

    public static let urlBarIndicatorTitle = NSLocalizedString(
      "focusOnboarding.urlBarIndicatorTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "See the Brave difference",
      comment: "The title of the popup which points to URL Bar after onboarding"
    )

    public static let urlBarIndicatorDescription = NSLocalizedString(
      "focusOnboarding.urlBarIndicatorDescription",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Enter a URL to enjoy Fewer ads & trackers",
      comment: "The description of the popup which points to URL Bar after onboarding"
    )
  }
}
