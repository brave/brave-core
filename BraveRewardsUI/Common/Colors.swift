/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveUI

extension UIColor {
  convenience init(hex: UInt32) {
    let r = CGFloat((hex & 0xFF0000) >> 16) / 255.0
    let g = CGFloat((hex & 0x00FF00) >> 8) / 255.0
    let b = CGFloat(hex & 0x0000FF) / 255.0
    self.init(displayP3Red: r, green: g, blue: b, alpha: 1.0)
  }
}

/// Collection of common color usages
struct BraveUX {
  static let braveOrange = Colors.orange500
  static let switchOnColor = Colors.blurple400
  static let autoContributeTintColor = UIColor(hex: 0x90329C) // No close color in Brave palette
  static let tipsTintColor = UIColor(hex: 0x6A71D5)  // No close color in Brave palette
  static let adsTintColor = UIColor(hex: 0xB13677) // No close color in Brave palette
  static let upholdGreen = UIColor(hex: 0x1bba6a)
}
