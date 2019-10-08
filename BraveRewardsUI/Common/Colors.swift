/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

extension UIColor {
  convenience init(hex: UInt32) {
    let r = CGFloat((hex & 0xFF0000) >> 16) / 255.0
    let g = CGFloat((hex & 0x00FF00) >> 8) / 255.0
    let b = CGFloat(hex & 0x0000FF) / 255.0
    self.init(displayP3Red: r, green: g, blue: b, alpha: 1.0)
  }
}

class Colors {
  static let grey000 = UIColor(hex: 0x1E2029)
  static let grey100 = UIColor(hex: 0x3B3E4F)
  static let grey200 = UIColor(hex: 0x5E6175)
  static let grey300 = UIColor(hex: 0x84889C)
  static let grey400 = UIColor(hex: 0xAEB1C2)
  static let grey600 = UIColor(hex: 0xCED0DB)
  static let grey900 = UIColor(hex: 0xF0F2FF)
  static let neutral200 = UIColor(hex: 0x495057)
  static let neutral600 = UIColor(hex: 0xDEE2E6)
  static let neutral800 = UIColor(hex: 0xF1F3F5)
  static let blue500 = UIColor(hex: 0x5DB5FC)
  static let blurple000 = UIColor(hex: 0x0B0E38)
  static let blurple400 = UIColor(hex: 0x4C54D2)
  static let blurple500 = UIColor(hex: 0x737ADE)
  static let blurple600 = UIColor(hex: 0xA0A5EB)
  static let blurple700 = UIColor(hex: 0xD0D2F7)
  static let blurple800 = UIColor(hex: 0xF0F1FF)
  static let magenta300 = UIColor(hex: 0xA3278F)
  static let purple300 = UIColor(hex: 0x6845D1)
  static let purple400 = UIColor(hex: 0x845EF7)
  static let orange400 = UIColor(hex: 0xFB542B)
  static let red300 = UIColor(hex: 0xBD1531)
}

/// Collection of common color usages
struct BraveUX {
  static let braveOrange = Colors.orange400
  static let switchOnColor = Colors.blurple500
  static let autoContributeTintColor = UIColor(hex: 0x90329C) // No close color in Brave palette
  static let tipsTintColor = UIColor(hex: 0x6A71D5)  // No close color in Brave palette
  static let adsTintColor = UIColor(hex: 0xB13677) // No close color in Brave palette
  
}
