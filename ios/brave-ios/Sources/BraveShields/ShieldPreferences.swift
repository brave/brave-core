// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

extension Preferences {
  public final class Shields {
    public static let allShields = [
      googleSafeBrowsing, blockScripts, fingerprintingProtection, blockImages,
    ]
    /// Enable Google Safe Browsing
    public static let googleSafeBrowsing = Option<Bool>(
      key: "shields.google-safe-browsing",
      default: true
    )
    /// Disables JavaScript execution in the browser
    public static let blockScripts = Option<Bool>(key: "shields.block-scripts", default: false)
    /// Enforces fingerprinting protection on the users session
    public static let fingerprintingProtection = Option<Bool>(
      key: "shields.fingerprinting-protection",
      default: true
    )
    /// Enable redirecting of Google's AMP (Accelerated Mobile Page) to the original (non-AMP) pages
    public static let autoRedirectAMPPagesDeprecated = Option<Bool?>(
      key: "shields.auto-redirect-amp-pages",
      default: nil
    )
    /// Enable redirecting of tracking urls (i.e. debouncing)
    public static let autoRedirectTrackingURLsDeprecated = Option<Bool?>(
      key: "shields.auto-redirect-tracking-urls",
      default: nil
    )
    /// Disables image loading in the browser
    public static let blockImages = Option<Bool>(key: "shields.block-images", default: false)
    /// In addition to global adblocking rules, adds custom country based rules.
    /// This setting is enabled by default for all locales.
    public static let useRegionAdBlock = Option<Bool>(
      key: "shields.regional-adblock",
      default: true
    )
    /// Version of downloaded data file for adblock stats.
    public static let adblockStatsDataVersion = Option<Int?>(
      key: "stats.adblock-data-version",
      default: nil
    )
    /// Whether or not advanced controls in the shields UI are visible by default
    public static let advancedControlsVisible = Option<Bool>(
      key: "shields.advanced-controls-visible",
      default: false
    )
    /// Whether or not we've reported the initial state of shields for p3a
    public static let initialP3AStateReportedRevision = Option<Int>(
      key: "shields.initial-p3a-state-reported-revision",
      default: 0
    )

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

    /// Get the auto shred level setting as a stored preference
    /// - Warning: You should not access this directly but  through ``shredLevel``
    static var shredLevelRaw = Preferences.Option<String?>(
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

    /// If Shred should also shred from user's history.
    public static var shredHistoryItems = Preferences.Option<Bool>(
      key: "shields.shred-history-items",
      default: false
    )
  }
}
