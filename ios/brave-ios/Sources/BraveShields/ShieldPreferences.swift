// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

/// All the settings pertaining to shields and privacy
public class ShieldPreferences {
  private static let defaultBlockAdsAndTrackingLevel: ShieldLevel = .standard
  private static let defaultHTTPsUpgradeLevel: HTTPSUpgradeLevel = .standard

  /// Get the level of the adblock and tracking protection as a stored preference
  /// - Warning: You should not access this directly but  through ``blockAdsAndTrackingLevel``
  public static var blockAdsAndTrackingLevelRaw = Preferences.Option<String>(
    key: "shields.block-ads-and-tracking-level",
    default: defaultBlockAdsAndTrackingLevel.rawValue
  )

  /// Get the level of the https upgrade setting as a stored preference
  /// - Warning: You should not access this directly but  through ``httpsUpgradeLevel``
  public static var httpsUpgradeLevelRaw = Preferences.Option<String>(
    key: "shields.https-upgrade-level",
    default: defaultHTTPsUpgradeLevel.rawValue
  )

  /// Get the enabled level for https upgrade for when the kBraveHttpsByDefault feature flag is off
  /// This preserves the value the user had set (possibly `strict` or `standard`) when they enable
  /// https everywhere
  public static var httpsUpgradePriorEnabledLevelRaw = Preferences.Option<String?>(
    key: "shields.https-upgrade-prior-enabled-level",
    default: nil
  )

  /// Get the level of the https upgrade setting as a stored preference
  /// - Warning: You should not access this directly but  through ``httpsUpgradeLevel``
  private static var shredLevelRaw = Preferences.Option<String?>(
    key: "shields.shred-level",
    default: nil
  )

  /// Get the level of the adblock and tracking protection
  public static var blockAdsAndTrackingLevel: ShieldLevel {
    get {
      ShieldLevel(rawValue: blockAdsAndTrackingLevelRaw.value) ?? defaultBlockAdsAndTrackingLevel
    }
    set { blockAdsAndTrackingLevelRaw.value = newValue.rawValue }
  }

  /// Get the level of HTTPS upgrades
  public static var httpsUpgradeLevel: HTTPSUpgradeLevel {
    get {
      HTTPSUpgradeLevel(rawValue: httpsUpgradeLevelRaw.value) ?? defaultHTTPsUpgradeLevel
    }
    set { httpsUpgradeLevelRaw.value = newValue.rawValue }
  }

  /// Get the prior enabled level of HTTPS upgrades
  public static var httpsUpgradePriorEnabledLevel: HTTPSUpgradeLevel? {
    get {
      httpsUpgradePriorEnabledLevelRaw.value.flatMap { HTTPSUpgradeLevel(rawValue: $0) }
    }
    set { httpsUpgradePriorEnabledLevelRaw.value = newValue?.rawValue }
  }

  /// Get the global shred level value
  public static var shredLevel: SiteShredLevel {
    get {
      guard let shredLevelRaw = self.shredLevelRaw.value else { return .never }
      return SiteShredLevel(rawValue: shredLevelRaw) ?? .never
    }
    set {
      shredLevelRaw.value = newValue.rawValue
    }
  }

  /// A boolean value inidicating if GPC is enabled
  public static var enableGPC = Preferences.Option<Bool>(
    key: "shields.enable-gpc",
    default: true
  )
}
