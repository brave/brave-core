// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Preferences
import Growth

public enum FullScreenCalloutType: CaseIterable {
  case bottomBar, p3a, rewards, defaultBrowser, blockCookieConsentNotices, vpnPromotion, vpnLinkReceipt

  /// The number of days passed to show certain type of callout
  var period: Int {
    switch self {
    case .bottomBar: return 0
    case .p3a: return 0
    case .rewards: return 8
    case .defaultBrowser: return 10
    case .blockCookieConsentNotices: return 0
    case .vpnPromotion: return 4
    case .vpnLinkReceipt: return 0
    }
  }

  /// The preference value stored for complete state
  public var preferenceValue: Preferences.Option<Bool> {
    switch self {
    case .bottomBar:
      return Preferences.FullScreenCallout.bottomBarCalloutCompleted
    case .p3a:
      return Preferences.Onboarding.p3aOnboardingShown
    case .rewards:
      return Preferences.FullScreenCallout.rewardsCalloutCompleted
    case .defaultBrowser:
      return Preferences.DefaultBrowserIntro.completed
    case .blockCookieConsentNotices:
      return Preferences.FullScreenCallout.blockCookieConsentNoticesCalloutCompleted
    case .vpnPromotion:
      return Preferences.FullScreenCallout.vpnCalloutCompleted
    case .vpnLinkReceipt:
      return Preferences.Onboarding.vpnLinkReceiptShown
    }
  }
}

public struct FullScreenCalloutManager {
  /// It determines whether we should show show the designated callout or not and sets corresponding preferences accordingly.
  /// Returns true if the callout should be shown.
  public static func shouldShowDefaultBrowserCallout(calloutType: FullScreenCalloutType) -> Bool {
    guard Preferences.Onboarding.isNewRetentionUser.value == true,
      let appRetentionLaunchDate = Preferences.DAU.appRetentionLaunchDate.value,
      !calloutType.preferenceValue.value
    else {
      return false
    }

    let rightNow = Date()

    let nextShowDate = appRetentionLaunchDate.addingTimeInterval(AppConstants.buildChannel.isPublic ? calloutType.period.days : calloutType.period.minutes)

    if rightNow > nextShowDate {
      calloutType.preferenceValue.value = true
      return true
    }

    return false
  }
}
