// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Strings
import BraveShields
import UIKit
import BraveWidgetsModels

struct StatData {
  var name: String
  var value: String
  var color: UIColor = .braveLabel
}

extension StatKind {
  var valueColor: UIColor {
    switch self {
    case .adsBlocked:
      return UIColor(rgb: 0xFB542B)
    case .dataSaved:
      return UIColor(rgb: 0xA0A5EB)
    case .timeSaved:
      return .braveLabel
    case .unknown:
      return .braveLabel
    @unknown default:
      assertionFailure()
      return .braveLabel
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

  var displayString: String {
    switch self {
    case .adsBlocked:
      return BraveGlobalShieldStats.shared.adblock.kFormattedNumber
    case .dataSaved:
      return BraveGlobalShieldStats.shared.dataSaved
    case .timeSaved:
      return BraveGlobalShieldStats.shared.timeSaved
    case .unknown:
      return ""
    @unknown default:
      assertionFailure()
      return ""
    }
  }
}
