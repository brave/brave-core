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

    public static let noVideoAdsScreenTitle = NSLocalizedString(
      "focusOnboarding.noVideoAdsScreenTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Block Interruptions When You Stream",
      comment: "The title of the screen that shows ads embeded in video are blocked"
    )

    public static let noVideoAdsScreenDescription = NSLocalizedString(
      "focusOnboarding.noVideoAdsScreenDescription",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Block annoying interruptions on your favorite streaming sites.",
      comment: "The subtitle of the screen that shows ads embeded in video are blocked"
    )

    public static let p3aScreenTitle = NSLocalizedString(
      "focusOnboarding.p3aScreenTitle",
      tableName: "FocusOnboarding",
      bundle: .module,
      value: "Make Brave Better",
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
      value: "Share **completely private & anonymous** product insights.",
      comment:
        "The title of the toggle for enable / disable the privacy preserving analytics. This uses standard markdown syntax. The \"completely private & anonymous\" snippet should be bold in all languages"
    )

    public static let p3aToggleDescription = NSLocalizedString(
      "focusOnboarding.p3aToggleDescription",
      tableName: "FocusOnboarding",
      bundle: .module,
      value:
        "You can opt-out any time in Settings under **Shields and Privacy**. [Learn more](#p3a-learn-more) about our Privacy Preserving Product Analytics.",
      comment:
        "The description shown below the toggle for enabling privacy preserving analytics. This uses standard markdown syntax. The Learn More is a link and #p3a-learn-more is a URL fragment"
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
      value: "Set As Default",
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
      value: "Start streaming and enjoy fewer ads & trackers.",
      comment: "The description of the popup which points to URL Bar after onboarding"
    )
  }
}
