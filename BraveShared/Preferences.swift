/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

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
@objc public protocol PreferencesObserver: class {
    /// A preference value was changed for some given preference key
    func preferencesDidChange(for key: String)
}

extension Preferences {
    public final class DAU {
        public static let lastLaunchInfo = Option<[Int?]?>(key: "dau.last-launch-info", default: nil)
        static let weekOfInstallation = Option<String?>(key: "dau.week-of-installation", default: nil)
        // On old codebase we checked existence of `dau_stat` to determine whether it's first server ping.
        // We need to translate that to use the new `firstPingParam` preference.
        static let firstPingParam: Option<Bool> =
            Option<Bool>(key: "dau.first-ping", default: Preferences.DAU.lastLaunchInfo.value == nil)
    }
    final class URP {
        static let nextCheckDate = Option<TimeInterval?>(key: "urp.next-check-date", default: nil)
        static let retryCountdown = Option<Int?>(key: "urp.retry-countdown", default: nil)
        static let customHeaderData = Option<Data?>(key: "urp.custom-header-data", default: nil)
        static let downloadId = Option<String?>(key: "urp.referral.download-id", default: nil)
        static let referralCode = Option<String?>(key: "urp.referral.code", default: nil)
        static let referralCodeDeleteDate = Option<TimeInterval?>(key: "urp.referral.delete-date", default: nil)
    }
    
    public final class NTP {
        public static let ntpCheckDate = Option<TimeInterval?>(key: "ntp.next-check-date", default: nil)
    }
    
    public final class Review {
        /// Application Launch Count (how many times the application has been launched)
        public static let launchCount = Option<Int>(key: "review.launch-count", default: 0)
        /// Review Threshold (the total amount of launches needed for the next review to show up)
        static let threshold = Option<Int>(key: "review.threshold", default: AppReview.firstThreshold)
        /// Last Review Date
        static let lastReviewDate = Option<Date?>(key: "review.last-date", default: nil)
    }
    
    final class BlockStats {
        static let adsCount = Option<Int>(key: "stats.adblock", default: 0)
        static let trackersCount = Option<Int>(key: "stats.tracking", default: 0)
        static let scriptsCount = Option<Int>(key: "stats.scripts", default: 0)
        static let imagesCount = Option<Int>(key: "stats.images", default: 0)
        static let phishingCount = Option<Int>(key: "stats.phishing", default: 0)
        static let httpsUpgradeCount = Option<Int>(key: "stats.http-upgrade", default: 0)
        static let fingerprintingCount = Option<Int>(key: "stats.fingerprinting", default: 0)
    }
    public final class BlockFileVersion {
        public static let adblock = Option<String?>(key: "blockfile.adblock", default: nil)
        public static let httpse = Option<String?>(key: "blockfile.httpse", default: nil)
    }
    
    public final class Shields {
        public static let allShields = [blockAdsAndTracking, httpsEverywhere, blockPhishingAndMalware, blockScripts, fingerprintingProtection, blockImages]
        
        /// Shields will block ads and tracking if enabled
        public static let blockAdsAndTracking = Option<Bool>(key: "shields.block-ads-and-tracking", default: true)
        /// Websites will be upgraded to HTTPS if a loaded page attempts to use HTTP
        public static let httpsEverywhere = Option<Bool>(key: "shields.https-everywhere", default: true)
        /// Shields will block websites related to potential phishing and malware
        public static let blockPhishingAndMalware = Option<Bool>(key: "shields.block-phishing-and-malware", default: true)
        /// Disables JavaScript execution in the browser
        public static let blockScripts = Option<Bool>(key: "shields.block-scripts", default: false)
        /// Enforces fingerprinting protection on the users session
        public static let fingerprintingProtection = Option<Bool>(key: "shields.fingerprinting-protection", default: false)
        /// Disables image loading in the browser
        public static let blockImages = Option<Bool>(key: "shields.block-images", default: false)
        /// In addition to global adblocking rules, adds custom country based rules.
        /// This setting is enabled by default for all locales.
        public static let useRegionAdBlock = Option<Bool>(key: "shields.regional-adblock", default: true)
        /// Version of downloaded data file for adblock stats.
        public static let adblockStatsDataVersion = Option<Int?>(key: "stats.adblock-data-version", default: nil)
    }
    
    public final class Rewards {
        public static let myFirstAdShown = Option<Bool>(key: "rewards.ads.my-first-ad-shown", default: false)
        public static let hideRewardsIcon = Option<Bool>(key: "rewards.hide-rewards-icon", default: false)
        public static let panelOpened = Option<Bool>(key: "rewards.rewards-panel-opened", default: false)
        public static let isUsingBAP = Option<Bool?>(key: "rewards.is-using-bap", default: nil)
        public static let checkedPreviousCycleForAdsViewing = Option<Bool>(key: "rewards.checked-previous-ads-cycle", default: false)
        public static let seenDataMigrationFailureError = Option<Bool>(key: "rewards.seen-data-migration-failure-error", default: false)
        
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
        public static let environmentOverride = Option<Int>(key: "rewards.environment-override",
                                                            default: EnvironmentOverride.none.rawValue)
        
        /// In debut/beta, the number of seconds before an ad should automatically dismiss
        public static let adsDurationOverride = Option<Int?>(key: "rewards.ads.dismissal-override", default: nil)
        
        /// Whether or not the user successfully enrolled before
        public static let didEnrollDeviceCheck = Option<Bool>(key: "rewards.devicecheck.did.enroll", default: false)
    }
}

extension Preferences {
    
    /// An entry in the `Preferences`
    ///
    /// `ValueType` defines the type of value that will stored in the UserDefaults object
    public class Option<ValueType: UserDefaultsEncodable & Equatable> {
        /// The list of observers for this option
        private let observers = WeakList<PreferencesObserver>()
        /// The UserDefaults container that you wish to save to
        public let container: UserDefaults
        /// The current value of this preference
        ///
        /// Upon setting this value, UserDefaults will be updated and any observers will be called
        public var value: ValueType {
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

extension Preferences {
    /// Migrate a given key from `Prefs` into a specific option
    public class func migrate<T>(keyPrefix: String, key: String, to option: Preferences.Option<T>, transform: ((T) -> T)? = nil) {
        let userDefaults = UserDefaults(suiteName: AppInfo.sharedContainerIdentifier)

        let profileKey = "\(keyPrefix)\(key)"
        // Have to do two checks because T may be an Optional, since object(forKey:) returns Any? it will succeed
        // as casting to T if T is Optional even if the key doesnt exist.
        let value = userDefaults?.object(forKey: profileKey)
        if value != nil, let value = value as? T {
            if let transform = transform {
                option.value = transform(value)
            } else {
                option.value = value
            }
            userDefaults?.removeObject(forKey: profileKey)
        } else {
            Logger.browserLogger.info("Could not migrate legacy pref with key: \"\(profileKey)\".")
        }
    }
    
    public class func migrateBraveShared(keyPrefix: String) {
        // DAU
        migrate(keyPrefix: keyPrefix, key: "dau_stat", to: Preferences.DAU.lastLaunchInfo)
        migrate(keyPrefix: keyPrefix, key: "week_of_installation", to: Preferences.DAU.weekOfInstallation)
        
        // URP
        migrate(keyPrefix: keyPrefix, key: "urpDateCheckPrefsKey", to: Preferences.URP.nextCheckDate)
        migrate(keyPrefix: keyPrefix, key: "urpRetryCountdownPrefsKey", to: Preferences.URP.retryCountdown)
        migrate(keyPrefix: keyPrefix, key: "CustomHeaderDataPrefs", to: Preferences.URP.customHeaderData)
        migrate(keyPrefix: keyPrefix, key: "downloadIdPrefsKey", to: Preferences.URP.downloadId)
        migrate(keyPrefix: keyPrefix, key: "referralCodePrefsKey", to: Preferences.URP.referralCode)
        migrate(keyPrefix: keyPrefix, key: "referralCodeDeleteTimePrefsKey", to: Preferences.URP.referralCodeDeleteDate)
        
        // Block Stats
        migrate(keyPrefix: keyPrefix, key: "adblock", to: Preferences.BlockStats.adsCount)
        migrate(keyPrefix: keyPrefix, key: "tracking_protection", to: Preferences.BlockStats.trackersCount)
        migrate(keyPrefix: keyPrefix, key: "httpse", to: Preferences.BlockStats.httpsUpgradeCount)
        migrate(keyPrefix: keyPrefix, key: "fp_protection", to: Preferences.BlockStats.fingerprintingCount)
        migrate(keyPrefix: keyPrefix, key: "safebrowsing", to: Preferences.BlockStats.phishingCount)
    }
}

