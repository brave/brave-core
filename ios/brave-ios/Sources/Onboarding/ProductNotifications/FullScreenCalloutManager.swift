// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Growth
import Preferences
import Shared

public enum FullScreenCalloutType: CaseIterable {
  // The order will effect the priority
  //
  // Priority:
  // - P3A
  // - VPN Update Billing
  // - Bottom Bar
  // - VPN Promotion
  // - Default Browser
  // - Rewards
  // - VPN Link Receipt
  case p3a, vpnUpdateBilling, bottomBar, vpnPromotion, defaultBrowser, rewards, vpnLinkReceipt

  /// The number of days passed to show certain type of callout
  var period: Int {
    switch self {
    case .p3a: return 0
    case .vpnUpdateBilling: return 0
    case .bottomBar: return 0
    case .vpnPromotion: return 4
    case .defaultBrowser: return Locale.current.isNewOnboardingRegion ? 7 : 10
    case .rewards: return 8
    case .vpnLinkReceipt: return 0
    }
  }

  /// The preference value stored for complete state
  public var preferenceValue: Preferences.Option<Bool> {
    switch self {
    case .p3a:
      return Preferences.Onboarding.p3aOnboardingShown
    case .vpnUpdateBilling:
      return Preferences.FullScreenCallout.vpnUpdateBillingCalloutCompleted
    case .bottomBar:
      return Preferences.FullScreenCallout.bottomBarCalloutCompleted
    case .vpnPromotion:
      return Preferences.FullScreenCallout.vpnPromotionCalloutCompleted
    case .defaultBrowser:
      return Preferences.DefaultBrowserIntro.completed
    case .rewards:
      return Preferences.FullScreenCallout.rewardsCalloutCompleted
    case .vpnLinkReceipt:
      return Preferences.Onboarding.vpnLinkReceiptShown
    }
  }
}

public struct FullScreenCalloutManager {
  ///  Delay period 3 days that will be added to full screen callouts or  product notifications
  public static let delayAmountJpOnboarding = 3.days

  /// It determines whether we should show show the designated callout or not and sets corresponding preferences accordingly.
  /// Returns true if the callout should be shown.
  public static func shouldShowCallout(calloutType: FullScreenCalloutType) -> Bool {
    // If region is onboarding_region check new focus onboarding is finished
    if Locale.current.isNewOnboardingRegion,
      !Preferences.FocusOnboarding.focusOnboardingFinished.value
    {
      return false
    }

    guard Preferences.Onboarding.isNewRetentionUser.value == true,
      let appRetentionLaunchDate = Preferences.DAU.appRetentionLaunchDate.value,
      !calloutType.preferenceValue.value
    else {
      return false
    }

    let rightNow = Date()
    var calloutDelayInterval = calloutType.period.days

    // Delay period 3 days that will be added to full screen callouts
    // This will be the case as long as new onboarding is active for onboarding_regions
    if Locale.current.isNewOnboardingRegion {
      calloutDelayInterval += delayAmountJpOnboarding
    }

    let nextShowDate = appRetentionLaunchDate.addingTimeInterval(calloutDelayInterval)

    if rightNow > nextShowDate {
      calloutType.preferenceValue.value = true
      return true
    }

    return false
  }
}
