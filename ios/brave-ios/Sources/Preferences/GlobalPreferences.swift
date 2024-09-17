// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Prefs that are accessed across multiple different targets
///
/// Some of these should move to ClientPreferences or their own targets preferences file in the future
extension Preferences {
  public final class BlockStats {
    public static let adsCount = Option<Int>(key: "stats.adblock", default: 0)
    public static let trackersCount = Option<Int>(key: "stats.tracking", default: 0)
    public static let scriptsCount = Option<Int>(key: "stats.scripts", default: 0)
    public static let imagesCount = Option<Int>(key: "stats.images", default: 0)
    public static let phishingCount = Option<Int>(key: "stats.phishing", default: 0)
    public static let fingerprintingCount = Option<Int>(key: "stats.fingerprinting", default: 0)
  }

  public final class BlockFileVersion {
    public static let adblock = Option<String?>(key: "blockfile.adblock", default: nil)
    public static let httpse = Option<String?>(key: "blockfile.httpse", default: nil)
  }

  public final class ProductNotificationBenchmarks {
    public static let videoAdBlockShown = Option<Bool>(
      key: "product-benchmark.videoAdBlockShown",
      default: false
    )
    public static let trackerTierCount = Option<Int>(
      key: "product-benchmark.trackerTierCount",
      default: 0
    )
    public static let showingSpecificDataSavedEnabled = Option<Bool>(
      key: "product-benchmark.showingSpecificDataSavedEnabled",
      default: false
    )
  }

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
  }

  public final class Rewards {
    public static let hideRewardsIcon = Option<Bool>(
      key: "rewards.new-hide-rewards-icon",
      default: false
    )
    public static let rewardsToggledOnce = Option<Bool>(
      key: "rewards.rewards-toggled-once",
      default: false
    )
    public static let isUsingBAP = Option<Bool?>(key: "rewards.is-using-bap", default: nil)
    public static let adaptiveCaptchaFailureCount = Option<Int>(
      key: "rewards.adaptive-captcha-failure-count",
      default: 0
    )
    public static let adsEnabledTimestamp = Option<Date?>(
      key: "rewards.ads.last-time-enabled",
      default: nil
    )
    public static let adsDisabledTimestamp = Option<Date?>(
      key: "rewards.ads.last-time-disabled",
      default: nil
    )

    public enum EnvironmentOverride: Int {
      case none
      case staging
      case prod
      case dev

      public var name: String {
        switch self {
        case .none: return "None"
        case .staging: return "Staging"
        case .prod: return "Prod"
        case .dev: return "Dev"
        }
      }

      public static var sortedCases: [EnvironmentOverride] {
        return [.none, .dev, .staging, .prod]
      }
    }
    /// In debug/beta, this is the overriden environment.
    public static let environmentOverride = Option<Int>(
      key: "rewards.environment-override",
      default: EnvironmentOverride.none.rawValue
    )

    public static let debugFlagIsDebug = Option<Bool?>(key: "rewards.flag.is-debug", default: nil)
    public static let debugFlagRetryInterval = Option<Int?>(
      key: "rewards.flag.retry-interval",
      default: nil
    )
    public static let debugFlagReconcileInterval = Option<Int?>(
      key: "rewards.flag.reconcile-interval",
      default: nil
    )

    /// In debut/beta, the number of seconds before an ad should automatically dismiss
    public static let adsDurationOverride = Option<Int?>(
      key: "rewards.ads.dismissal-override",
      default: nil
    )

    /// Whether or not the user successfully enrolled before
    public static let didEnrollDeviceCheck = Option<Bool>(
      key: "rewards.devicecheck.did.enroll",
      default: false
    )
  }

  public final class BraveCore {
    /// Switches that are passed into BraveCoreMain during launch.
    ///
    /// This preference stores a list of `BraveCoreSwitch` raw values
    public static let activeSwitches = Option<[String]>(
      key: "brave-core.active-switches",
      default: []
    )
    /// The values for switches that may be passed into BraveCoreMain if they're active
    ///
    /// Each key is a `BraveCoreSwitch`
    public static let switchValues = Option<[String: String]>(
      key: "brave-core.switches.values",
      default: [:]
    )
    /// Custom Switches that are passed into BraveCoreMain during launch.
    ///
    /// This preference stores a list of `BraveCoreSwitch` raw values
    public static let customSwitches = Option<[String]>(
      key: "brave-core.custom-switches",
      default: []
    )
  }

  public final class AppState {
    /// A flag for determining if the app exited with user interaction in the previous session
    ///
    /// Value should only be checked on launch
    public static let backgroundedCleanly = Option<Bool>(
      key: "appstate.backgrounded-cleanly",
      default: true
    )

    /// A cached value for the last folder path we got for filter lists
    ///
    /// This is a useful setting because it take too long for filter lists to load during launch
    /// and therefore we can try to load them right away and have them ready on the first tab load
    @MainActor public static let lastLegacyDefaultFilterListFolderPath =
      Option<String?>(key: "caching.last-default-filter-list-folder-path", default: nil)

    /// A cached value for the last folder path we got for our ad-block resources
    ///
    /// This is a useful setting because it take too long for filter lists to load during launch
    /// and therefore we can try to load them right away and have them ready on the first tab load
    @MainActor public static let lastAdBlockResourcesFolderPath =
      Option<String?>(key: "caching.last-ad-block-resources-folder-path", default: nil)

    /// A cached value for the last file path we got for our ad-block resources
    ///
    /// This is a useful setting because it takes too long for resources to load during launch
    /// and therefore we can try to load them right away and have them ready on the first tab load
    @MainActor public static let lastAdBlockResourcesFilePath =
      Option<String?>(key: "caching.last-ad-block-resources-file-path", default: nil)

    /// A cached value for the last folder path we got our filter lists components
    ///
    /// This is a useful setting because it take too long for filter lists to load during launch
    /// and therefore we can try to load them right away and have them ready on the first tab load
    @MainActor public static let lastFilterListCatalogueComponentFolderPath =
      Option<String?>(key: "caching.last-filter-list-catalogue-component-folder-path", default: nil)

    /// A cached value for indicating if onboarding is actively going on
    ///
    /// This is used to determine if a promoted purchase from store can be triggered and shown user
    public static let shouldDeferPromotedPurchase = Option<Bool>(
      key: "appstate.onboarding-active",
      default: false
    )

    /// A cached value for indicating if dau ping is awaiting for p3a choice on onboarding
    ///
    /// This is used to determine if a user gave consent to p3a in onboarding and dau ping can fetch referral code from Apple API
    public static let dailyUserPingAwaitingUserConsent = Option<Bool>(
      key: "appstate.dau-awaiting",
      default: false
    )
  }

  public final class Chromium {
    /// Whether the device is in sync chain
    public static let syncEnabled = Option<Bool>(key: "chromium.sync.enabled", default: false)
    /// The sync type bookmarks enabled for the device in sync chain
    public static let syncBookmarksEnabled = Option<Bool>(
      key: "chromium.sync.syncBookmarksEnabled",
      default: true
    )
    /// The sync type history enabled for the device in sync chain
    public static let syncHistoryEnabled = Option<Bool>(
      key: "chromium.sync.syncHistoryEnabled",
      default: false
    )
    /// The sync type passwords enabled the device in sync chain
    public static let syncPasswordsEnabled = Option<Bool>(
      key: "chromium.sync.syncPasswordsEnabled",
      default: false
    )
    /// The sync type open tabs enabled the device in sync chain
    public static let syncOpenTabsEnabled = Option<Bool>(
      key: "chromium.sync.openTabsEnabled",
      default: false
    )
    /// Node Id for last bookmark folder
    public static let lastBookmarksFolderNodeId = Option<Int?>(
      key: "chromium.last.bookmark.folder.node.id",
      default: nil
    )
  }

  public final class Debug {
    /// Whether or not the settings page shows developer options
    public static let developerOptionsEnabled = Option<Bool>(
      key: "debug.dev-options-enabled",
      default: false
    )
  }
}
