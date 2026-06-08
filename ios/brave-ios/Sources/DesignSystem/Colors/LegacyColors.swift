// swift-format-ignore-file

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

enum LegacyDesignSystemColor: String {
  case interactive01
  case interactive02
  case interactive03
  case interactive04
  case interactive05
  case interactive06
  case interactive07
  case interactive08

  case gradient01_step0 = "gradient01-step0"
  case gradient01_step1 = "gradient01-step1"

  case gradient02_step0 = "gradient02-step0"
  case gradient02_step1 = "gradient02-step1"
  case gradient02_step2 = "gradient02-step2"

  case gradient05_step0 = "gradient05-step0"
  case gradient05_step1 = "gradient05-step1"

  var color: UIColor {
    return UIColor(named: rawValue, in: .module, compatibleWith: nil)!
  }
}

// MARK: - Design System Colors

extension UIColor {
  public static var braveLighterOrange: UIColor {
    LegacyDesignSystemColor.interactive03.color
  }
  public static var braveOrange: UIColor {
    LegacyDesignSystemColor.interactive02.color
  }
  public static var braveDarkerOrange: UIColor {
    LegacyDesignSystemColor.interactive01.color
  }
  public static var braveLighterBlurple: UIColor {
    LegacyDesignSystemColor.interactive06.color
  }
  public static var braveBlurple: UIColor {
    LegacyDesignSystemColor.interactive05.color
  }
  public static var braveDarkerBlurple: UIColor {
    LegacyDesignSystemColor.interactive04.color
  }
  public static var braveBlurpleTint: UIColor {
    .init {
      if $0.userInterfaceStyle == .dark {
        return .braveLighterBlurple
      }
      return .braveBlurple
    }
  }
  public static var primaryButtonTint: UIColor {
    LegacyDesignSystemColor.interactive07.color
  }
  public static var secondaryButtonTint: UIColor {
    LegacyDesignSystemColor.interactive08.color
  }
}

// MARK: - Static Colors

extension UIColor {
  public static var statsAdsBlockedTint: UIColor {
    .braveOrange
  }
  public static var statsDataSavedTint: UIColor {
    .init(hex: 0xA0A5EB)
  }
  public static var statsTimeSavedTint: UIColor {
    .white
  }
}

extension UIColor {
  fileprivate convenience init(hex: UInt32) {
    let r = CGFloat((hex & 0xFF0000) >> 16) / 255.0
    let g = CGFloat((hex & 0x00FF00) >> 8) / 255.0
    let b = CGFloat(hex & 0x0000FF) / 255.0
    self.init(displayP3Red: r, green: g, blue: b, alpha: 1.0)
  }
}
