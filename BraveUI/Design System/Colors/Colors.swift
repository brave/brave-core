// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

/// The Brave Color Palette used in Brave's Design System
///
/// File: `Color Palette.sketch`
/// Version: `9dc5d63`
final public class Colors {
  // MARK: - Neutral
  public static let neutral000 = UIColor(hex: 0xF8F9Fa)
  public static let neutral100 = UIColor(hex: 0xF1F3F5)
  public static let neutral300 = UIColor(hex: 0xDEE2E6)
  public static let neutral700 = UIColor(hex: 0x495057)
  // MARK: - Grey
  public static let grey000 = UIColor(hex: 0xF0F2FF)
  public static let grey100 = UIColor(hex: 0xE6E8F5)
  public static let grey200 = UIColor(hex: 0xDADCE8)
  public static let grey300 = UIColor(hex: 0xCED0DB)
  public static let grey400 = UIColor(hex: 0xC2C4CF)
  public static let grey500 = UIColor(hex: 0xAEB1C2)
  public static let grey600 = UIColor(hex: 0x84889C)
  public static let grey700 = UIColor(hex: 0x5E6175)
  public static let grey800 = UIColor(hex: 0x3B3E4F)
  public static let grey900 = UIColor(hex: 0x1E2029)
  // MARK: - Red
  public static let red600 = UIColor(hex: 0xBD1531)
  // MARK: - Magenta
  public static let magenta600 = UIColor(hex: 0xA3278F)
  // MARK: - Purple
  public static let purple500 = UIColor(hex: 0x845EF7)
  public static let purple600 = UIColor(hex: 0x6845D1)
  // MARK: - Blurple
  public static let blurple100 = UIColor(hex: 0xF0F1FF)
  public static let blurple200 = UIColor(hex: 0xD0D2F7)
  public static let blurple300 = UIColor(hex: 0xA0A5EB)
  public static let blurple400 = UIColor(hex: 0x737ADE)
  public static let blurple500 = UIColor(hex: 0x4C54D2)
  public static let blurple900 = UIColor(hex: 0x0B0E38)
  // MARK: - Blue
  public static let blue400 = UIColor(hex: 0x5DB5FC)
  // MARK: - Orange
  public static let orange400 = UIColor(hex: 0xFF7654)
  public static let orange500 = UIColor(hex: 0xFB542B)
}

extension UIColor {
  fileprivate convenience init(hex: UInt32) {
    let r = CGFloat((hex & 0xFF0000) >> 16) / 255.0
    let g = CGFloat((hex & 0x00FF00) >> 8) / 255.0
    let b = CGFloat(hex & 0x0000FF) / 255.0
    self.init(displayP3Red: r, green: g, blue: b, alpha: 1.0)
  }
}
