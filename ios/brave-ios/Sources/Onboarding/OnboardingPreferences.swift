// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences

extension Preferences {
  public final class Onboarding {
    /// Whether or not new user onboarding has completed.
    /// User skipping(tapping on skip) onboarding does NOT count as completed.
    /// If user kills the app before completing onboarding, it should be treated as unfinished.
    public static let basicOnboardingCompleted = Option<Int>(
      key: "general.basic-onboarding-completed",
      default: OnboardingState.undetermined.rawValue
    )

    /// The time until the next on-boarding shows
    public static let basicOnboardingDefaultBrowserSelected = Option<Bool>(
      key: "general.basic-onboarding-default-browser-selected",
      default: false
    )

    /// The progress the user has made with onboarding
    public static let basicOnboardingProgress = Option<Int>(
      key: "general.basic-onboarding-progress",
      default: OnboardingProgress.none.rawValue
    )

    /// The bool detemining if p3a infomartion is shown in onboarding to a user so they will not see it again as pop-over
    public static let p3aOnboardingShown = Option<Bool>(
      key: "onboarding.basic-onboarding-default-browser-selected",
      default: false
    )

    /// Whether the link vpn receipt alert has been shown.
    public static let vpnLinkReceiptShown = Option<Bool>(
      key: "onboarding.link-receipt",
      default: false
    )

    /// Whether this is a new user who installed the application after onboarding retention updates
    public static let isNewRetentionUser = Option<Bool?>(key: "general.new-retention", default: nil)
  }
}

extension Preferences {
  public final class FocusOnboarding {
    /// The bool detemining if URL bar onboarding should be shown NTP
    public static let urlBarIndicatorShowBeShown = Option<Bool>(
      key: "focus.onboarding.url-bar-indicator",
      default: false
    )

    /// The Bool determining onboarding finished fully
    public static let focusOnboardingFinished = Option<Bool>(
      key: "focus.onboarding.onboarding-finished",
      default: false
    )
  }
}

extension Preferences {
  public final class FullScreenCallout {

    /// Whether the bottom bar callout is shown.
    public static let bottomBarCalloutCompleted = Option<Bool>(
      key: "fullScreenCallout.full-screen-bottom-bar-callout-completed",
      default: false
    )

    /// Whether the rewards callout is shown.
    public static let rewardsCalloutCompleted = Option<Bool>(
      key: "fullScreenCallout.full-screen-rewards-callout-completed",
      default: false
    )

    /// Whether the ntp callout is shown.
    public static let ntpCalloutCompleted = Option<Bool>(
      key: "fullScreenCallout.full-screen-ntp-callout-completed",
      default: false
    )

    /// Whether the omnibox callout is shown.
    public static let omniboxCalloutCompleted = Option<Bool>(
      key: "fullScreenCallout.full-screen-omnibox-callout-completed",
      default: false
    )

    /// Whether the vpn promotion callout is shown.
    public static let vpnUpdateBillingCalloutCompleted = Option<Bool>(
      key: "fullScreenCallout.full-screen-vpn-billing-callout-completed",
      default: false
    )
  }
}

extension Preferences {
  public final class DefaultBrowserIntro {
    /// Whether the default browser onboarding completed. This can happen by opening app settings or after the user
    /// dismissed the intro screen enough amount of times.
    public static let completed = Option<Bool>(
      key: "defaultBrowserIntro.intro-completed",
      default: false
    )

    /// Whether system notification scheduled or not
    public static let defaultBrowserNotificationScheduled = Option<Bool>(
      key: "general.default-browser-notification-scheduled",
      default: false
    )

    /// Whether a default browser local notification should be shown
    public static let defaultBrowserNotificationIsCanceled = Option<Bool>(
      key: "general.default-browser-notification-canceled",
      default: false
    )
  }
}
