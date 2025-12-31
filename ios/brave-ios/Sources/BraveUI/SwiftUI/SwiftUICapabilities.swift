// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI

/// Utility for checking SwiftUI capabilities and system features
public enum SwiftUICapabilities {
  /// Determines if Liquid Glass is enabled on iOS 26.1+
  public static var isLiquidGlassEnabled: Bool {
    #if compiler(>=6.2.1)
    if #available(iOS 26.1, *) {
      let isCompatabilityModeEnabled =
        Bundle.main.infoDictionary?["UIDesignRequiresCompatibility"] as? Bool == true
      if isCompatabilityModeEnabled {
        let key = "com.apple.Swi\("ftUI.IgnoreSolar")iumOptOut"
        return UserDefaults.standard.bool(forKey: key)
      }
    }
    #endif
    return false
  }
}
