// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// Utility for checking SwiftUI capabilities and system features
public enum LiquidGlassMode {
  /// Determines if Liquid Glass is enabled on iOS 26+
  public static var isEnabled: Bool {
    if #available(iOS 26, *) {
      let isCompatabilityModeEnabled =
        Bundle.main.infoDictionary?["UIDesignRequiresCompatibility"] as? Bool == true
      if isCompatabilityModeEnabled {
        let key = "com.apple.Swi\("ftUI.IgnoreSolar")iumOptOut"
        return UserDefaults.standard.bool(forKey: key)
      }
      return true
    }
    return false
  }
}

extension UIView {
  /// Returns the appropriate layout guide for horizontal constraints, accounting for iOS 26+ Liquid Glass mode
  /// When Liquid Glass is enabled, returns a layout guide with corner adaptation for horizontal constraints suitable for window controls
  public var liquidGlassHorizontalSafeAreaLayoutGuide: UILayoutGuide {
    if #available(iOS 26.0, *), LiquidGlassMode.isEnabled {
      return layoutGuide(for: .safeArea(cornerAdaptation: .horizontal))
    }
    return safeAreaLayoutGuide
  }
}

struct LiquidGlassLayoutTrafficLightPadding: ViewModifier {
  func body(content: Content) -> some View {
    if #available(iOS 26.0, *) {
      content
        .containerCornerOffset(.leading, sizeToFit: true)
    } else {
      content
    }
  }
}

extension View {
  public func liquidGlassLayoutTrafficLightPadding() -> some View {
    modifier(LiquidGlassLayoutTrafficLightPadding())
  }
}
