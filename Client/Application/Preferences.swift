/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared

/// An empty protocol simply here to force the developer to use a user defaults encodable value via generic constraint
protocol UserDefaultsEncodable {}

enum TabBarVisibility: Int {
    case never
    case always
    case landscapeOnly
    
    // TODO: Remove when we can use Swift 4.2/`CaseIterable`
    static let allCases: [TabBarVisibility] = [.never, .always, .landscapeOnly]
}

enum PasswordManagerShortcutBehavior: Int {
    case showPicker
    case onePassword
    case lastPass
    case bitwarden
    case trueKey
    
    // TODO: Remove when we can use Swift 4.2/`CaseIterable`
    static let allCases: [PasswordManagerShortcutBehavior] = [.showPicker, .onePassword, .lastPass, .bitwarden, .trueKey]
}

/// The applications preferences container
///
/// Properties in this object should be of the the type `Option` with the object which is being
/// stored to automatically interact with `UserDefaults`
final class Preferences {
    /// The default `UserDefaults` that all `Option`s will use unless specified
    static let defaultContainer = UserDefaults(suiteName: AppInfo.sharedContainerIdentifier)!
}

// MARK: - Other Preferences
extension Preferences {
    final class Popups {
        /// Whether or not the user has opted-in for using DuckDuckGo during a Private Browsing session
        ///
        /// Defaults to nil, meaning the user has not been given the choice yet
        static let duckDuckGoPrivateSearch = Option<Bool?>(key: "popups.ddg-private-search", default: nil)
    }
}

// MARK: - User Preferences
extension Preferences {
    final class General {
        /// Whether or not to save logins in Brave
        static let saveLogins = Option<Bool>(key: "general.save-logins", default: true)
        /// Whether or not to block popups from websites automaticaly
        static let blockPopups = Option<Bool>(key: "general.block-popups", default: true)
        /// Controls how the tab bar should be shown (or not shown)
        static let tabBarVisibility = Option<Int>(key: "general.tab-bar-visiblity", default: TabBarVisibility.always.rawValue)
        /// The behavior when the password manager button is tapped
        static let passwordManagerShortcutBehavior = Option<Int>(key: "general.pm-button-behavior", default: PasswordManagerShortcutBehavior.showPicker.rawValue)
    }
    final class Search {
        /// Whether or not to show suggestions while the user types
        static let showSuggestions = Option<Bool>(key: "search.show-suggestions", default: false)
        /// A list of disabled search engines
        static let disabledEngines = Option<[String]>(key: "search.disabled-engines", default: [])
        /// A list of ordered search engines or nil if they have not been set up yet
        static let orderedEngines = Option<[String]?>(key: "search.ordered-engines", default: nil)
    }
    final class Privacy {
        /// Forces all private tabs
        static let privateBrowsingOnly = Option<Bool>(key: "privacy.private-only", default: false)
        /// The users preference on how to store Cookies
        static let cookieAcceptPolicy = Option<UInt>(key: "privacy.cookie-accept", default: HTTPCookie.AcceptPolicy.onlyFromMainDocumentDomain.rawValue)
        /// The toggles states for clear private data screen
        static let clearPrivateDataToggles = Option<[Bool]>(key: "privacy.clear-data-toggles", default: [])
    }
    final class Shields {
        /// Shields will block ads and tracking if enabled
        static let blockAdsAndTracking = Option<Bool>(key: "shields.block-ads-and-tracking", default: true)
        /// Websites will be upgraded to HTTPS if a loaded page attempts to use HTTP
        static let httpsEverywhere = Option<Bool>(key: "shields.https-everywhere", default: true)
        /// Shields will block websites related to potential phishing and malware
        static let blockPhishingAndMalware = Option<Bool>(key: "shields.block-phishing-and-malware", default: true)
        /// Disables JavaScript execution in the browser
        static let blockScripts = Option<Bool>(key: "shields.block-scripts", default: false)
        /// Enforces fingerprinting protection on the users session
        static let fingerprintingProtection = Option<Bool>(key: "shields.fingerprinting-protection", default: false)
        ///
        static let useRegionAdBlock = Option<Bool>(key: "shields.regional-adblock", default: false)
    }
    final class Support {
        /// Whether or not the user has opted in to sending crash reports to Brave
        static let sendsCrashReportsAndMetrics = Option<Bool>(key: "support.send-reports", default: true)
    }
}

/// Defines an object which may watch a set of `Preference.Option`s
/// - note: @objc was added here due to a Swift compiler bug which doesn't allow a class-bound protocol
/// to act as `AnyObject` in a `AnyObject` generic constraint (i.e. `WeakList`)
@objc protocol PreferencesObserver: class {
    /// A preference value was changed for some given preference key
    func preferencesDidChange(for key: String)
}

extension Preferences {
    
    /// An entry in the `Preferences`
    ///
    /// `ValueType` defines the type of value that will stored in the UserDefaults object
    class Option<ValueType: UserDefaultsEncodable> {
        /// The list of observers for this option
        private let observers = WeakList<PreferencesObserver>()
        /// The UserDefaults container that you wish to save to
        private let container: UserDefaults
        /// The current value of this preference
        ///
        /// Upon setting this value, UserDefaults will be updated and any observers will be called
        var value: ValueType {
            didSet {
                container.set(value, forKey: self.key)
                container.synchronize()
                
                let key = self.key
                observers.forEach {
                    $0.preferencesDidChange(for: key)
                }
            }
        }
        /// Adds `object` as an observer for this Option.
        func observe(from object: PreferencesObserver) {
            observers.insert(object)
        }
        /// The key used for getting/setting the value in `UserDefaults`
        let key: String
        /// Creates a preference
        init(key: String, default: ValueType, container: UserDefaults = Preferences.defaultContainer) {
            self.key = key
            self.container = container
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
extension Array: UserDefaultsEncodable where Element: UserDefaultsEncodable {}
extension Dictionary: UserDefaultsEncodable where Key: StringProtocol, Value: UserDefaultsEncodable {}
