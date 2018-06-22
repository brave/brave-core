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
}

/// The applications preferences container
///
/// Properties in this object should be of the the type `Option` with the object which is being
/// stored to automatically interact with `UserDefaults`
final class Preferences {
  static let saveLogins = Option<Bool>(key: "general.save-logins", default: true)
  static let blockPopups = Option<Bool>(key: "general.block-popups", default: true)
  static let tabBarVisibility = Option<Int>(key: "general.tab-bar-visiblity", default: TabBarVisibility.always.rawValue)
}

extension Preferences {
  final class Search {
    static let showSuggestions = Option<Bool>(key: "search.show-suggestions", default: false)
    static let disabledEngines = Option<[String]>(key: "search.disabled-engines", default: [])
    static let orderedEngines = Option<[String]>(key: "search.ordered-engines", default: [])
  }
  final class Privacy {
    static let privateBrowsingOnly = Option<Bool>(key: "privacy.private-only", default: false)
  }
  final class Security {
    static let browserLockEnabled = Option<Bool>(key: "security.browser-lock", default: false)
  }
  final class Shields {
    static let blockAdsAndTracking = Option<Bool>(key: "shields.block-ads-and-tracking", default: true)
    static let httpsEverywhere = Option<Bool>(key: "shields.https-everywhere", default: true)
    static let blockPhishingAndMalware = Option<Bool>(key: "shields.block-phishing-and-malware", default: true)
    static let blockScripts = Option<Bool>(key: "shields.block-scripts", default: false)
    static let fingerprintingProtection = Option<Bool>(key: "shields.fingerprinting-protection", default: false)
    static let useRegionAdBlock = Option<Bool>(key: "shields.regional-adblock", default: false)
  }
  final class Support {
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
        container.set(value, forKey: key)
        container.synchronize()
        
        if observers.count() > 0 {
          let key = self.key
          observers.forEach {
            $0.preferencesDidChange(for: key)
          }
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
    fileprivate init(key: String, default: ValueType, container: UserDefaults = UserDefaults.standard) {
      self.key = key
      self.container = container
      value = (container.value(forKey: key) as? ValueType) ?? `default`
    }
  }
}

extension Optional: UserDefaultsEncodable where Wrapped: UserDefaultsEncodable {}
extension Bool: UserDefaultsEncodable {}
extension Int: UserDefaultsEncodable {}
extension Float: UserDefaultsEncodable {}
extension Double: UserDefaultsEncodable {}
extension String: UserDefaultsEncodable {}
extension URL: UserDefaultsEncodable {}
extension Data: UserDefaultsEncodable {}
extension Array: UserDefaultsEncodable where Element: UserDefaultsEncodable {}
extension Dictionary: UserDefaultsEncodable where Key: StringProtocol, Value: UserDefaultsEncodable {}
