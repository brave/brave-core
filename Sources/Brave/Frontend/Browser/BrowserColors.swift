// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import DesignSystem
import BraveUI

/// A set of browser theme colors found in the `🦁 Browser` Figma doc. There will be a separate set depending
/// on the browsing mode you're in (standard vs private)
protocol BrowserColors {
  // MARK: - Text
  var textPrimary: UIColor { get }
  var textSecondary: UIColor { get }
  var textTertiary: UIColor { get }
  var textDisabled: UIColor { get }
  var textInteractive: UIColor { get }
  
  // MARK: - Icon
  var iconDefault: UIColor { get }
  var iconDisabled: UIColor { get }
  var iconActive: UIColor { get }
  
  // MARK: - BrowserButton
  var browserButtonBackgroundHover: UIColor { get }
  var browserButtonBackgroundActive: UIColor { get }
  
  // MARK: - TabSwitcher
  var tabSwitcherButton: UIColor { get }
  var tabSwitcherBackground: UIColor { get }
  
  // MARK: - Container
  var containerBackground: UIColor { get }
  var containerInteractive: UIColor { get }
  var containerScrim: UIColor { get }
  var containerFrostedGlass: UIColor { get }
  
  // MARK: - Chrome
  var chromeBackground: UIColor { get }
  
  // MARK: - Divider
  var dividerSubtle: UIColor { get }
  var dividerStrong: UIColor { get }
  
  // MARK: - TabBar
  var tabBarTabBackground: UIColor { get }
  var tabBarTabActiveBackground: UIColor { get }
}

extension BrowserColors where Self == StandardBrowserColors {
  /// The standard set of light & dark mode browser colors
  static var standard: StandardBrowserColors { .init() }
}

extension BrowserColors where Self == PrivateModeBrowserColors {
  /// The set of browser colors specific to private mode
  static var privateMode: PrivateModeBrowserColors { .init() }
}

/// The standard set of light & dark mode browser colors
struct StandardBrowserColors: BrowserColors {
  var textPrimary: UIColor {
    .init(light: .primitiveGray100, dark: .primitiveGray1)
  }
  
  var textSecondary: UIColor {
    .init(light: .primitiveGray70, dark: .primitiveGray30)
  }
  
  var textTertiary: UIColor {
    .init(light: .primitiveGray50, dark: .primitiveGray40)
  }
  
  var textDisabled: UIColor {
    .init(lightRGBA: 0x21242A80, darkRGBA: 0xEDEEF180)
  }
  
  var textInteractive: UIColor {
    .init(light: .primitivePrimary60, dark: .primitivePrimary40)
  }
  
  var iconDefault: UIColor {
    .init(light: .primitiveGray50, dark: .primitiveGray40)
  }
  
  var iconDisabled: UIColor {
    .init(lightRGBA: 0x68748580, darkRGBA: 0xA1ABBA80)
  }
  
  var iconActive: UIColor {
    .init(light: .primitivePrimary60, dark: .primitivePrimary40)
  }
  
  var browserButtonBackgroundHover: UIColor {
    .init(light: .primitiveGray10, dark: .primitiveGray80)
  }
  
  var browserButtonBackgroundActive: UIColor {
    .init(light: .primitiveGray20, dark: .primitiveGray100)
  }
  
  var tabSwitcherButton: UIColor {
    .init(lightColor: .white, darkColor: .init(braveSystemName: .primitiveGray90))
  }
  
  var tabSwitcherBackground: UIColor {
    .init(light: .primitiveGray1, dark: .primitiveGray80)
  }
  
  var containerBackground: UIColor {
    .init(lightColor: .white, darkColor: .init(braveSystemName: .primitiveGray90))
  }
  
  var containerInteractive: UIColor {
    .init(light: .primitivePrimary10, dark: .primitivePrimary80)
  }
  
  var containerScrim: UIColor {
    .init(lightRGBA: 0x0D0F1459, darkRGBA: 0x0D0F14B3)
  }
  
  var containerFrostedGlass: UIColor {
    .init(lightRGBA: 0xFFFFFFF7, darkRGBA: 0x21242AEB)
  }
  
  var chromeBackground: UIColor {
    .init(light: .primitiveGray1, dark: .primitiveGray100)
  }
  
  var dividerSubtle: UIColor {
    .init(lightRGBA: 0xA1ABBA66, darkRGBA: 0x68748566)
  }
  
  var dividerStrong: UIColor {
    .init(lightRGBA: 0x68748566, darkRGBA: 0xA1ABBA66)
  }
  
  var tabBarTabBackground: UIColor {
    .init(light: .primitiveGray10, dark: .primitiveGray100)
  }
  
  var tabBarTabActiveBackground: UIColor {
    .init(light: .primitiveGray1, dark: .primitiveGray90)
  }
}

/// The set of browser colors specific to private mode
struct PrivateModeBrowserColors: BrowserColors {
  var textPrimary: UIColor {
    .init(braveSystemName: .primitivePrivateWindow1)
  }
  
  var textSecondary: UIColor {
    .init(braveSystemName: .primitivePrivateWindow30)
  }
  
  var textTertiary: UIColor {
    .init(braveSystemName: .primitivePrivateWindow40)
  }
  
  var textDisabled: UIColor {
    .init(rgba: 0xEEEBFF80)
  }
  
  var textInteractive: UIColor {
    .init(braveSystemName: .primitivePrimary40)
  }
  
  var iconDefault: UIColor {
    .init(braveSystemName: .primitivePrivateWindow40)
  }
  
  var iconDisabled: UIColor {
    .init(rgba: 0xA380FF80)
  }
  
  var iconActive: UIColor {
    .init(braveSystemName: .primitivePrimary40)
  }
  
  var browserButtonBackgroundHover: UIColor {
    .init(braveSystemName: .primitivePrivateWindow80)
  }
  
  var browserButtonBackgroundActive: UIColor {
    .init(braveSystemName: .primitivePrivateWindow100)
  }
  
  var tabSwitcherButton: UIColor {
    .init(braveSystemName: .primitivePrimary90)
  }
  
  var tabSwitcherBackground: UIColor {
    .init(braveSystemName: .primitivePrimary80)
  }
  
  var containerBackground: UIColor {
    .init(braveSystemName: .primitivePrivateWindow90)
  }
  
  var containerInteractive: UIColor {
    .init(braveSystemName: .primitivePrimary90)
  }
  
  var containerScrim: UIColor {
    .init(rgba: 0x13052AB3)
  }
  
  var containerFrostedGlass: UIColor {
    .init(rgba: 0x2A0D58EB)
  }
  
  var chromeBackground: UIColor {
    .init(braveSystemName: .primitivePrivateWindow100)
  }
  
  var dividerSubtle: UIColor {
    .init(rgba: 0x7B63BF66)
  }
  
  var dividerStrong: UIColor {
    .init(rgba: 0xA380FF66)
  }
  
  var tabBarTabBackground: UIColor {
    .init(braveSystemName: .primitivePrivateWindow100)
  }
  
  var tabBarTabActiveBackground: UIColor {
    .init(braveSystemName: .primitivePrivateWindow90)
  }
}

extension UIColor {
  fileprivate convenience init(light: FigmaColorResource, dark: FigmaColorResource) {
    self.init(dynamicProvider: { traitCollection in
      if traitCollection.userInterfaceStyle == .dark {
        return .init(braveSystemName: dark)
      }
      return .init(braveSystemName: light)
    })
  }
  
  fileprivate convenience init(lightColor: UIColor, darkColor: UIColor) {
    self.init(dynamicProvider: { traitCollection in
      if traitCollection.userInterfaceStyle == .dark {
        return darkColor
      }
      return lightColor
    })
  }
  
  fileprivate convenience init(lightRGBA: UInt32, darkRGBA: UInt32) {
    self.init(dynamicProvider: { traitCollection in
      if traitCollection.userInterfaceStyle == .dark {
        return .init(rgba: darkRGBA)
      }
      return .init(rgba: lightRGBA)
    })
  }
}
