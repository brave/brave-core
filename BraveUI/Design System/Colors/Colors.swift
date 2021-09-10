// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

enum DesignSystemColor: String {
  case background01
  case background02
  case background03
  
  case text01
  case text02
  case text03
  
  case interactive01
  case interactive02
  case interactive03
  case interactive04
  case interactive05
  case interactive06
  case interactive07
  case interactive08
  
  case focusBorder = "focus-border"
  case disabled
  
  case divider01
  
  case errorBackground = "error-background"
  case errorBorder = "error-border"
  case errorText = "error-text"
  
  case warningBackground = "warning-background"
  case warningBorder = "warning-border"
  case warningText = "warning-text"
  
  case infoBackground = "info-background"
  case infoBorder = "info-border"
  case infoText = "info-text"
  
  case successBackground = "success-background"
  case successBorder = "success-border"
  case successText = "success-text"
  
  case gradient01_step0 = "gradient01-step0"
  case gradient01_step1 = "gradient01-step1"
  
  case gradient02_step0 = "gradient02-step0"
  case gradient02_step1 = "gradient02-step1"
  case gradient02_step2 = "gradient02-step2"
  
  case gradient03_step0 = "gradient03-step0"
  case gradient03_step1 = "gradient03-step1"
  
  var color: UIColor {
    return UIColor(named: rawValue, in: Bundle(for: BraveUI.self), compatibleWith: nil)!
  }
}

final private class BraveUI { }

// MARK: - Design System Colors

extension UIColor {
  public static var braveBackground: UIColor {
    DesignSystemColor.background02.color
  }
  public static var secondaryBraveBackground: UIColor {
    DesignSystemColor.background01.color
  }
  public static var tertiaryBraveBackground: UIColor {
    DesignSystemColor.background03.color
  }
  public static var braveGroupedBackground: UIColor {
    DesignSystemColor.background01.color
  }
  public static var secondaryBraveGroupedBackground: UIColor {
    DesignSystemColor.background02.color
  }
  public static var tertiaryBraveGroupedBackground: UIColor {
    DesignSystemColor.background01.color
  }
  public static var braveLabel: UIColor {
    DesignSystemColor.text02.color
  }
  public static var secondaryBraveLabel: UIColor {
    DesignSystemColor.text03.color
  }
  public static var bravePrimary: UIColor {
    DesignSystemColor.text01.color
  }
  public static var braveLighterOrange: UIColor {
    DesignSystemColor.interactive03.color
  }
  public static var braveOrange: UIColor {
    DesignSystemColor.interactive02.color
  }
  public static var braveDarkerOrange: UIColor {
    DesignSystemColor.interactive01.color
  }
  public static var braveLighterBlurple: UIColor {
    DesignSystemColor.interactive06.color
  }
  public static var braveBlurple: UIColor {
    DesignSystemColor.interactive05.color
  }
  public static var braveDarkerBlurple: UIColor {
    DesignSystemColor.interactive04.color
  }
  public static var braveSeparator: UIColor {
    DesignSystemColor.divider01.color
  }
  public static var braveErrorLabel: UIColor {
    DesignSystemColor.errorText.color
  }
  public static var braveErrorBorder: UIColor {
    DesignSystemColor.errorBorder.color
  }
  public static var braveErrorBackground: UIColor {
    DesignSystemColor.errorBackground.color
  }
  public static var braveWarningLabel: UIColor {
    DesignSystemColor.warningText.color
  }
  public static var braveWarningBorder: UIColor {
    DesignSystemColor.warningBorder.color
  }
  public static var braveWarningBackground: UIColor {
    DesignSystemColor.warningBackground.color
  }
  public static var braveInfoLabel: UIColor {
    DesignSystemColor.infoText.color
  }
  public static var braveInfoBorder: UIColor {
    DesignSystemColor.infoBorder.color
  }
  public static var braveInfoBackground: UIColor {
    DesignSystemColor.infoBackground.color
  }
  public static var braveSuccessLabel: UIColor {
    DesignSystemColor.successText.color
  }
  public static var braveSuccessBackground: UIColor {
    DesignSystemColor.successBackground.color
  }
  public static var braveDisabled: UIColor {
    DesignSystemColor.disabled.color
  }
  public static var primaryButtonTint: UIColor {
    DesignSystemColor.interactive07.color
  }
  public static var secondaryButtonTint: UIColor {
    DesignSystemColor.interactive08.color
  }
}

// MARK: - Static Colors

extension UIColor {
  public static var privateModeBackground: UIColor {
    .init(hex: 0x2C2153)
  }
  public static var secondaryPrivateModeBackground: UIColor {
    .init(hex: 0x0D0920)
  }
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
