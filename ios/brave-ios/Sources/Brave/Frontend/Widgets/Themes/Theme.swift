/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveUI
import Shared
import UIKit

public enum DefaultTheme: String, RepresentableOptionType {
  case system = "Z71ED37E-EC3E-436E-AD5F-B22748306A6B"
  case light = "ACE618A3-D6FC-45A4-94F2-1793C40AE927"
  case dark = "B900A41F-2C02-4664-9DE4-C170956339AC"

  public static var normalThemesOptions = [
    DefaultTheme.system,
    DefaultTheme.light,
    DefaultTheme.dark,
  ]

  public var userInterfaceStyleOverride: UIUserInterfaceStyle {
    switch self {
    case .system:
      return .unspecified
    case .light:
      return .light
    case .dark:
      return .dark
    }
  }

  public var displayString: String {
    // Due to translations needs, titles are hardcoded here, ideally they would be pulled from the
    //  theme files themselves.
    switch self {
    case .system: return Strings.themesAutomaticOption
    case .light: return Strings.themesLightOption
    case .dark: return Strings.themesDarkOption
    }
  }
}
