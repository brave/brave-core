// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

public class ShieldPreferences {
  private static let defaultBlockAdsAndTrackingLevel: ShieldLevel = .standard
  
  /// Get the level of the adblock and tracking protection as a stored preference
  /// - Warning: You should not access this directly but  through ``blockAdsAndTrackingLevel``
  public static var blockAdsAndTrackingLevelRaw = Preferences.Option<String>(
    key: "shields.block-ads-and-tracking-level",
    default: defaultBlockAdsAndTrackingLevel.rawValue
  )
  
  /// Get the level of the adblock and tracking protection
  public static var blockAdsAndTrackingLevel: ShieldLevel {
    get { ShieldLevel(rawValue: blockAdsAndTrackingLevelRaw.value) ?? defaultBlockAdsAndTrackingLevel }
    set { blockAdsAndTrackingLevelRaw.value = newValue.rawValue }
  }
  
  /// A boolean value inidicating if GPC is enabled
  public static var enableGPC = Preferences.Option<Bool>(
    key: "shields.enable-gpc",
    default: true
  )
}
