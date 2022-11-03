/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import os.log

/// An empty protocol simply here to force the developer to use a user defaults encodable value via generic constraint
public protocol UserDefaultsEncodable {}

/// The applications preferences container
///
/// Properties in this object should be of the the type `Option` with the object which is being
/// stored to automatically interact with `UserDefaults`
public class Preferences {
  /// The default `UserDefaults` that all `Option`s will use unless specified
  public static let defaultContainer = UserDefaults(suiteName: AppInfo.sharedContainerIdentifier)!
}

/// Defines an object which may watch a set of `Preference.Option`s
/// - note: @objc was added here due to a Swift compiler bug which doesn't allow a class-bound protocol
/// to act as `AnyObject` in a `AnyObject` generic constraint (i.e. `WeakList`)
@objc public protocol PreferencesObserver: AnyObject {
  /// A preference value was changed for some given preference key
  func preferencesDidChange(for key: String)
}

extension Preferences {
  public final class NTP {
    public static let ntpCheckDate = Option<TimeInterval?>(key: "ntp.next-check-date", default: nil)
  }

  public final class BraveNews {
    public static let isShowingOptIn = Option<Bool>(key: "brave-today.showing-opt-in", default: false)
    public static let userOptedIn = Option<Bool>(key: "brave-today.user-opted-in", default: false)
    public static let isEnabled = Option<Bool>(key: "brave-today.enabled", default: true)
    public static let languageChecked = Option<Bool>(key: "brave-today.language-checked", default: false)
    public static let debugEnvironment = Option<String?>(key: "brave-today.debug.environment", default: nil)
    public static let debugLanguageCode = Option<String?>(key: "brave-today.debug.language-code", default: nil)
  }

  public final class Review {
    /// Application Launch Count (how many times the application has been launched)
    public static let launchCount = Option<Int>(key: "review.launch-count", default: 0)
    public static let braveNewsCriteriaPassed = Option<Bool>(key: "review.brave-new.criteria", default: false)
    public static let numberBookmarksAdded =  Option<Int>(key: "review.numberBookmarksAdded", default: 0)
    public static let numberPlaylistItemsAdded =  Option<Int>(key: "review.numberPlaylistItemsAdded", default: 0)
    public static let dateWalletConnectedToDapp =  Option<Date?>(key: "review.connect-dapp.wallet", default: nil)
    public static let daysInUse = Option<[Date]>(key: "review.in-use", default: [])
  }

  public final class BlockStats {
    public static let adsCount = Option<Int>(key: "stats.adblock", default: 0)
    public static let trackersCount = Option<Int>(key: "stats.tracking", default: 0)
    static let scriptsCount = Option<Int>(key: "stats.scripts", default: 0)
    static let imagesCount = Option<Int>(key: "stats.images", default: 0)
    public static let phishingCount = Option<Int>(key: "stats.phishing", default: 0)
    public static let httpsUpgradeCount = Option<Int>(key: "stats.http-upgrade", default: 0)
    public static let fingerprintingCount = Option<Int>(key: "stats.fingerprinting", default: 0)
  }
  public final class BlockFileVersion {
    public static let adblock = Option<String?>(key: "blockfile.adblock", default: nil)
    public static let httpse = Option<String?>(key: "blockfile.httpse", default: nil)
  }

  public final class ProductNotificationBenchmarks {
    public static let videoAdBlockShown = Option<Bool>(key: "product-benchmark.videoAdBlockShown", default: false)
    public static let trackerTierCount = Option<Int>(key: "product-benchmark.trackerTierCount", default: 0)
    public static let showingSpecificDataSavedEnabled = Option<Bool>(key: "product-benchmark.showingSpecificDataSavedEnabled", default: false)
  }

  public final class Shields {
    public static let allShields = [blockAdsAndTracking, httpsEverywhere, blockPhishingAndMalware, googleSafeBrowsing, blockScripts, fingerprintingProtection, blockImages]

    /// Shields will block ads and tracking if enabled
    public static let blockAdsAndTracking = Option<Bool>(key: "shields.block-ads-and-tracking", default: true)
    /// Websites will be upgraded to HTTPS if a loaded page attempts to use HTTP
    public static let httpsEverywhere = Option<Bool>(key: "shields.https-everywhere", default: true)
    /// Enable Google Safe Browsing
    public static let googleSafeBrowsing = Option<Bool>(key: "shields.google-safe-browsing", default: true)
    /// Shields will block websites related to potential phishing and malware
    public static let blockPhishingAndMalware = Option<Bool>(key: "shields.block-phishing-and-malware", default: true)
    /// Disables JavaScript execution in the browser
    public static let blockScripts = Option<Bool>(key: "shields.block-scripts", default: false)
    /// Enforces fingerprinting protection on the users session
    public static let fingerprintingProtection = Option<Bool>(key: "shields.fingerprinting-protection", default: true)
    /// Enable redirecting of Google's AMP (Accelerated Mobile Page) to the original (non-AMP) pages
    public static let autoRedirectAMPPages = Option<Bool>(key: "shields.auto-redirect-amp-pages", default: true)
    /// Disables image loading in the browser
    public static let blockImages = Option<Bool>(key: "shields.block-images", default: false)
    /// In addition to global adblocking rules, adds custom country based rules.
    /// This setting is enabled by default for all locales.
    public static let useRegionAdBlock = Option<Bool>(key: "shields.regional-adblock", default: true)
    /// Version of downloaded data file for adblock stats.
    public static let adblockStatsDataVersion = Option<Int?>(key: "stats.adblock-data-version", default: nil)
    /// Whether or not advanced controls in the shields UI are visible by default
    public static let advancedControlsVisible = Option<Bool>(key: "shields.advanced-controls-visible", default: false)
  }

  public final class Rewards {
    public static let myFirstAdShown = Option<Bool>(key: "rewards.ads.my-first-ad-shown", default: false)
    public static let hideRewardsIcon = Option<Bool>(key: "rewards.new-hide-rewards-icon", default: false)
    public static let rewardsToggledOnce = Option<Bool>(key: "rewards.rewards-toggled-once", default: false)
    public static let isUsingBAP = Option<Bool?>(key: "rewards.is-using-bap", default: nil)
    public static let seenDataMigrationFailureError = Option<Bool>(key: "rewards.seen-data-migration-failure-error", default: false)
    public static let migratedLegacyWallet = Option<Bool>(key: "rewards.migrated-legacy-wallet", default: false)
    public static let dismissedLegacyWalletTransfer = Option<Bool>(key: "rewards.dismissed-legacy-wallet-transfer", default: false)
    public static let transferDrainID = Option<String?>(key: "rewards.legacy-wallet-transfer-drain-id", default: nil)
    public static let lastTransferStatus = Option<Int?>(key: "rewards.legacy-wallet-transfer-status", default: nil)
    public static let lastTransferStatusDismissed = Option<Int?>(key: "rewards.legacy-wallet-transfer-last-dismissed-status", default: nil)
    public static let transferCompletionAcknowledged = Option<Bool>(key: "rewards.legacy-wallet-transfer-completion-acknowledged", default: false)
    public static let transferUnavailableLastSeen = Option<TimeInterval?>(key: "rewards.transfer-unavailable-warning-last-seen-date", default: nil)
    public static let drainStatusOverride = Option<Int?>(key: "rewards.drain-status-override", default: nil)
    public static let adaptiveCaptchaFailureCount = Option<Int>(key: "rewards.adaptive-captcha-failure-count", default: 0)

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
      default: EnvironmentOverride.none.rawValue)
    
    public static let debugFlagIsDebug = Option<Bool?>(key: "rewards.flag.is-debug", default: nil)
    public static let debugFlagRetryInterval = Option<Int?>(key: "rewards.flag.retry-interval", default: nil)
    public static let debugFlagReconcileInterval = Option<Int?>(key: "rewards.flag.reconcile-interval", default: nil)

    /// In debut/beta, the number of seconds before an ad should automatically dismiss
    public static let adsDurationOverride = Option<Int?>(key: "rewards.ads.dismissal-override", default: nil)

    /// Whether or not the user successfully enrolled before
    public static let didEnrollDeviceCheck = Option<Bool>(key: "rewards.devicecheck.did.enroll", default: false)
  }

  public final class BraveCore {
    /// Switches that are passed into BraveCoreMain during launch.
    ///
    /// This preference stores a list of `BraveCoreSwitch` raw values
    public static let activeSwitches = Option<[String]>(key: "brave-core.active-switches", default: [])
    /// The values for switches that may be passed into BraveCoreMain if they're active
    ///
    /// Each key is a `BraveCoreSwitch`
    public static let switchValues = Option<[String: String]>(key: "brave-core.switches.values", default: [:])
  }
  
  public final class AppState {
    /// A flag for determining if the app exited with user interaction in the previous session
    ///
    /// Value should only be checked on launch
    public static let backgroundedCleanly = Option<Bool>(key: "appstate.backgrounded-cleanly", default: true)
    
    /// A cached value for the last folder path we got for filter lists
    ///
    /// This is a useful setting because it take too long for filter lists to load during launch
    /// and therefore we can try to load them right away and have them ready on the first tab load
    public static let lastDefaultFilterListFolderPath =
      Option<String?>(key: "caching.last-default-filter-list-folder-path", default: nil)
  }
  
  public final class Chromium {
    /// The boolean determine Bookmark Migration is finished on client side
    public static let syncV2BookmarksMigrationCompleted = Option<Bool>(key: "chromium.migration.bookmarks", default: false)
    /// The boolean determine History Migration is finished on client side
    public static let syncV2HistoryMigrationCompleted = Option<Bool>(key: "chromium.migration.history", default: false)
    /// The boolean determine Password Migration is finished on client side
    public static let syncV2PasswordMigrationCompleted = Option<Bool>(key: "chromium.migration.password", default: false)
    /// The boolean determine Password Migration is started on client side
    public static let syncV2PasswordMigrationStarted = Option<Bool>(key: "chromium.migration.password.started", default: false)
    /// The count of how many times migration is performed on client side - the value increases with every fail attempt and after 3 tries migration marked as successful
    public static let syncV2ObjectMigrationCount = Option<Int>(key: "chromium.migration.attempt.count", default: 0)
    /// Whether the device is in sync chain
    public static let syncEnabled = Option<Bool>(key: "chromium.sync.enabled", default: false)
    /// The sync type bookmarks enabled for the device in sync chain
    public static let syncBookmarksEnabled = Option<Bool>(key: "chromium.sync.syncBookmarksEnabled", default: true)
    /// The sync type history enabled for the device in sync chain
    public static let syncHistoryEnabled = Option<Bool>(key: "chromium.sync.syncHistoryEnabled", default: false)
    /// The sync type passwords enabled the device in sync chain
    public static let syncPasswordsEnabled = Option<Bool>(key: "chromium.sync.syncPasswordsEnabled", default: false)
    /// The sync type open tabs enabled the device in sync chain
    public static let syncOpenTabsEnabled = Option<Bool>(key: "chromium.sync.openTabsEnabled", default: false)
    /// Node Id for last bookmark folder
    public static let lastBookmarksFolderNodeId = Option<Int?>(key: "chromium.last.bookmark.folder.node.id", default: nil)
  }
}

extension Preferences {

  /// An entry in the `Preferences`
  ///
  /// `ValueType` defines the type of value that will stored in the UserDefaults object
  public class Option<ValueType: UserDefaultsEncodable & Equatable>: ObservableObject {
    /// The list of observers for this option
    private let observers = WeakList<PreferencesObserver>()
    /// The UserDefaults container that you wish to save to
    public let container: UserDefaults
    /// The current value of this preference
    ///
    /// Upon setting this value, UserDefaults will be updated and any observers will be called
    @Published public var value: ValueType {
      didSet {
        if value == oldValue { return }

        // Check if `ValueType` is something that can be nil
        if value is ExpressibleByNilLiteral {
          // We have to use a weird workaround to determine if it can be placed in the UserDefaults.
          // `nil` (NSNull when its bridged to ObjC) can be placed in a dictionary, but not in UserDefaults.
          let dictionary = NSMutableDictionary(object: value, forKey: self.key as NSString)
          // If the value we pull out of the dictionary is NSNull, we know its nil and should remove it
          // from the UserDefaults rather than attempt to set it
          if let value = dictionary[self.key], value is NSNull {
            container.removeObject(forKey: self.key)
          } else {
            container.set(value, forKey: self.key)
          }
        } else {
          container.set(value, forKey: self.key)
        }
        container.synchronize()

        let key = self.key
        observers.forEach {
          $0.preferencesDidChange(for: key)
        }
      }
    }
    /// Adds `object` as an observer for this Option.
    public func observe(from object: PreferencesObserver) {
      observers.insert(object)
    }
    /// The key used for getting/setting the value in `UserDefaults`
    public let key: String
    /// The default value of this preference
    public let defaultValue: ValueType
    /// Reset's the preference to its original default value
    public func reset() {
      value = defaultValue
    }

    /// Creates a preference
    public init(key: String, default: ValueType, container: UserDefaults = Preferences.defaultContainer) {
      self.key = key
      self.container = container
      self.defaultValue = `default`
      value = (container.value(forKey: key) as? ValueType) ?? `default`
    }
  }
}

extension Optional: UserDefaultsEncodable where Wrapped: UserDefaultsEncodable {}
extension Bool: UserDefaultsEncodable {}
extension Int: UserDefaultsEncodable {}
extension UInt: UserDefaultsEncodable {}
extension Float: UserDefaultsEncodable {}
extension Double: UserDefaultsEncodable {}
extension String: UserDefaultsEncodable {}
extension URL: UserDefaultsEncodable {}
extension Data: UserDefaultsEncodable {}
extension Date: UserDefaultsEncodable {}
extension Array: UserDefaultsEncodable where Element: UserDefaultsEncodable {}
extension Dictionary: UserDefaultsEncodable where Key: StringProtocol, Value: UserDefaultsEncodable {}
