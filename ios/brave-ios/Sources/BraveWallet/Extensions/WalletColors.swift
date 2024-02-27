// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import UIKit

extension UIColor {

  static var walletGreen: UIColor {
    UIColor(rgb: 0x2ac194)
  }

  static var walletRed: UIColor {
    UIColor(rgb: 0xee6374)
  }
}

/// Tempoarily store Leo Design Colours used in Wallet v2 until we pull in all colours to DesignSystem.
enum WalletV2Design {
  // Light/Text/Primary rgba(28, 30, 38, 1)
  // Dark/Text/Primary rgba(237, 238, 241, 1)
  static let textPrimary = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor(
        red: 28 / 255,
        green: 30 / 255,
        blue: 38 / 255,
        alpha: 1
      )
    } else {
      return UIColor(
        red: 237 / 255,
        green: 238 / 255,
        blue: 241 / 255,
        alpha: 1
      )
    }
  })

  // Light/Text/Secondary rgba(102, 109, 137, 1)
  // Dark/Text/Secondary rgba(135, 141, 166, 1)
  static let textSecondary = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor(
        red: 102 / 255,
        green: 109 / 255,
        blue: 137 / 255,
        alpha: 1
      )
    } else {
      return UIColor(
        red: 135 / 255,
        green: 141 / 255,
        blue: 166 / 255,
        alpha: 1
      )
    }
  })

  // Light/Text/Tertiary rgba(126, 133, 160, 1)
  // Dark/Text/Tertiary rgba(97, 104, 132, 1)
  static let textTertiary = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor(
        red: 126 / 255,
        green: 133 / 255,
        blue: 160 / 255,
        alpha: 1
      )
    } else {
      return UIColor(
        red: 97 / 255,
        green: 104 / 255,
        blue: 132 / 255,
        alpha: 1
      )
    }
  })

  // Light/Text/Interactive rgba(66, 62, 238, 1)
  // Dark/Text/Interactive rgba(170, 168, 247, 1)
  static let textInteractive = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor(
        red: 66 / 255,
        green: 62 / 255,
        blue: 238 / 255,
        alpha: 1
      )
    } else {
      return UIColor(
        red: 170 / 255,
        green: 168 / 255,
        blue: 247 / 255,
        alpha: 1
      )
    }
  })

  // Light Theme/Text/text01 rgba(33, 37, 41, 1)
  static let text01: UIColor = .bravePrimary

  // Extended/Gray/20 rgba(225, 226, 232, 1) / rgba(38, 42, 51, 1)
  static let extendedGray20 = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor(
        red: 225 / 255,
        green: 226 / 255,
        blue: 232 / 255,
        alpha: 1
      )
    } else {
      return UIColor(
        red: 38 / 255,
        green: 42 / 255,
        blue: 51 / 255,
        alpha: 1
      )
    }
  })

  // Extended/Gray/50 rgba(84, 90, 113, 1) / rgba(168, 173, 191, 1)
  static let extendedGray50 = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor(
        red: 84 / 255,
        green: 90 / 255,
        blue: 113 / 255,
        alpha: 1
      )
    } else {
      return UIColor(
        red: 168 / 255,
        green: 173 / 255,
        blue: 191 / 255,
        alpha: 1
      )
    }
  })

  // Light/Container/Background rgba(255, 255, 255, 1)
  // Dark/Container/Background rgba(25, 27, 34, 1)
  static let containerBackground = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor.white
    } else {
      return UIColor(
        red: 25 / 255,
        green: 27 / 255,
        blue: 34 / 255,
        alpha: 1
      )
    }
  })

  // Light/Container/Highlight rgba(240, 241, 244, 1)
  // Dark/Container/Highlight rgba(13, 14, 18, 1)
  static let containerHighlight = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor(
        red: 240 / 255,
        green: 241 / 255,
        blue: 244 / 255,
        alpha: 1
      )
    } else {
      return UIColor(
        red: 13 / 255,
        green: 14 / 255,
        blue: 18 / 255,
        alpha: 1
      )
    }
  })

  // Light/Interaction/Button-primary-background rgba(66, 62, 238, 1)
  // Dark/Interaction/Button-primary-background rgba(66, 62, 238, 1)
  static let interactionButtonPrimaryBackground = UIColor(
    red: 66 / 255,
    green: 62 / 255,
    blue: 238 / 255,
    alpha: 1
  )

  // Light/Icon/Interactive rgba(95, 92, 241, 1)
  // Dark/Icon/Interactive rgba(135, 132, 244, 1)
  static let iconInteractive = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor(
        red: 95 / 255,
        green: 92 / 255,
        blue: 241 / 255,
        alpha: 1
      )
    } else {
      return UIColor(
        red: 135 / 255,
        green: 132 / 255,
        blue: 244 / 255,
        alpha: 1
      )
    }
  })

  // Light/Icon/Default rgba(102, 109, 137, 1)
  // Dark/Icon/Default rgba(135, 141, 166, 1)
  static let iconDefault = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor(
        red: 102 / 255,
        green: 109 / 255,
        blue: 137 / 255,
        alpha: 1
      )
    } else {
      return UIColor(
        red: 135 / 255,
        green: 141 / 255,
        blue: 166 / 255,
        alpha: 1
      )
    }
  })

  // Light/Divider/Subtle rgba(232, 233, 238, 1)
  // Dark/Divider/Subtle rgba(43, 46, 59, 1)
  static let dividerSubtle = UIColor(dynamicProvider: { traits in
    if traits.userInterfaceStyle == .light {
      return UIColor(
        red: 232 / 255,
        green: 233 / 255,
        blue: 238 / 255,
        alpha: 1
      )
    } else {
      return UIColor(
        red: 43 / 255,
        green: 46 / 255,
        blue: 59 / 255,
        alpha: 1
      )
    }
  })

  static let passwordWeakRed = UIColor(rgb: 0xd40033)

  static let passwordMediumYellow = UIColor(rgb: 0xbd9600)

  static let passwordStrongGreen = UIColor(rgb: 0x31803e)

  static let spamNFTLabelBackground = UIColor(
    red: 1,
    green: 209 / 255,
    blue: 207 / 255,
    alpha: 1
  )
}
