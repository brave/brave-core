// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
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

public enum NightModeSetting: String, CaseIterable, RepresentableOptionType {
  case off
  case on
  case followAppearance

  init(isEnabled: Bool, followsAppearance: Bool) {
    if followsAppearance {
      self = .followAppearance
    } else {
      self = isEnabled ? .on : .off
    }
  }

  func isEnabled(whenAppearanceIsDark appearanceIsDark: Bool) -> Bool {
    switch self {
    case .off: return false
    case .on: return true
    case .followAppearance: return appearanceIsDark
    }
  }

  public var displayString: String {
    switch self {
    case .off: return Strings.NightMode.offOption
    case .on: return Strings.NightMode.onOption
    case .followAppearance: return Strings.NightMode.followAppearanceOption
    }
  }
}
