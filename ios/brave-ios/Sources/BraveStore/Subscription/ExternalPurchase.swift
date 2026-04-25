// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

extension EnvironmentValues {
  @Entry public var allowExternalPurchaseLinks = ExternalPurchaseLinksSupport.isAllowed
}

public struct ExternalPurchaseLinksSupport {
  /// Whether or not we are allowed to display external purchase links to the user.
  public static var isAllowed: Bool {
    #if DEBUG
    return true
    #else
    if !FeatureList.kBraveAllowExternalPurchaseLinks.enabled {
      return false
    }
    let allowedRegionIdentifiers = ["US"]
    if let currentRegion = Locale.current.region?.identifier {
      return allowedRegionIdentifiers.contains(currentRegion)
    }
    return false
    #endif
  }
  public static var discountCode: String = "BRAVE30"
  public static var discountAmount: Double = 0.3
}
