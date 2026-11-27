// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShields
import BraveWidgetsModels
import Foundation
import Preferences
import Strings
import UIKit

struct StatData {
  var name: String
  var value: String
  var color: UIColor = UIColor(braveSystemName: .textPrimary)
}

extension StatKind {
  var valueColor: UIColor {
    switch self {
    case .adsBlocked:
      return UIColor(braveSystemName: .primitiveOrange70)
    case .dataSaved:
      return UIColor(braveSystemName: .primitiveBlurple70)
    case .timeSaved:
      return UIColor(braveSystemName: .textPrimary)
    case .unknown:
      return UIColor(braveSystemName: .textPrimary)
    @unknown default:
      assertionFailure()
      return UIColor(braveSystemName: .textPrimary)
    }
  }

  var name: String {
    switch self {
    case .adsBlocked:
      return Strings.Shields.shieldsAdAndTrackerStats
    case .dataSaved:
      return Strings.Shields.dataSavedStat
    case .timeSaved:
      return Strings.Shields.shieldsTimeStats
    case .unknown:
      return ""
    @unknown default:
      assertionFailure()
      return ""
    }
  }

  private var blockedCount: Int {
    Preferences.BlockStats.adsCount.value + Preferences.BlockStats.trackersCount.value
  }

  var displayString: String {
    switch self {
    case .adsBlocked:
      return Preferences.BlockStats.adsCount.value.kFormattedNumber
    case .dataSaved:
      return BraveGlobalShieldStats.dataSaved(blockedCount: blockedCount)
    case .timeSaved:
      return BraveGlobalShieldStats.timeSaved(blockedCount: blockedCount)
    case .unknown:
      return ""
    @unknown default:
      assertionFailure()
      return ""
    }
  }
}
