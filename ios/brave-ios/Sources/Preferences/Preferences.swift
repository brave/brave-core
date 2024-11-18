// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import Foundation
import Shared
import os.log

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

  /// An entry in the `Preferences`
  ///
  /// `ValueType` defines the type of value that will stored in the UserDefaults object
  public class Option<ValueType: Equatable>: ObservableObject {
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

        writePreferenceValue(container, key, value)

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

    private var writePreferenceValue: ((UserDefaults, String, ValueType) -> Void)

    fileprivate init(
      key: String,
      initialValue: ValueType,
      defaultValue: ValueType,
      container: UserDefaults = Preferences.defaultContainer,
      writePreferenceValue: @escaping (UserDefaults, String, ValueType) -> Void
    ) {
      self.key = key
      self.container = container
      self.value = initialValue
      self.defaultValue = defaultValue
      self.writePreferenceValue = writePreferenceValue
    }
  }
}

extension Preferences.Option {
  /// Creates a preference and fetches the initial value from the container the default way
  private convenience init(
    key: String,
    defaultValue: ValueType,
    container: UserDefaults
  ) where ValueType: UserDefaultsEncodable {
    let initialValue = (container.value(forKey: key) as? ValueType) ?? defaultValue
    self.init(
      key: key,
      initialValue: initialValue,
      defaultValue: defaultValue,
      container: container,
      writePreferenceValue: { $0.set($2, forKey: $1) }
    )
  }
  /// Creates a preference storing a user defaults supported value type
  public convenience init(
    key: String,
    default: ValueType,
    container: UserDefaults = Preferences.defaultContainer
  ) where ValueType: UserDefaultsEncodable {
    self.init(key: key, defaultValue: `default`, container: container)
  }
  /// Creates a preference storing an array of user defaults supported value types
  public convenience init<V>(
    key: String,
    default: ValueType,
    container: UserDefaults = Preferences.defaultContainer
  ) where V: UserDefaultsEncodable, ValueType == [V] {
    self.init(key: key, defaultValue: `default`, container: container)
  }
  /// Creates a preference storing an dictionary of user defaults supported value types
  public convenience init<K, V>(
    key: String,
    default: ValueType,
    container: UserDefaults = Preferences.defaultContainer
  ) where K: StringProtocol, V: UserDefaultsEncodable, ValueType == [K: V] {
    self.init(key: key, defaultValue: `default`, container: container)
  }
}

extension Preferences.Option where ValueType: ExpressibleByNilLiteral {
  /// Creates a preference and fetches the initial value from the container the default way
  private convenience init<V>(
    key: String,
    defaultValue: ValueType,
    container: UserDefaults
  ) where V: UserDefaultsEncodable, ValueType == V? {
    let initialValue = (container.value(forKey: key) as? V) ?? defaultValue
    self.init(
      key: key,
      initialValue: initialValue,
      defaultValue: defaultValue,
      container: container,
      writePreferenceValue: { container, key, value in
        if let value = value {
          container.set(value, forKey: key)
        } else {
          container.removeObject(forKey: key)
        }
      }
    )
  }
  /// Creates a preference storing an optional user defaults supported value type
  public convenience init<V>(
    key: String,
    default: ValueType,
    container: UserDefaults = Preferences.defaultContainer
  ) where ValueType == V?, V: UserDefaultsEncodable {
    self.init(key: key, defaultValue: `default`, container: container)
  }
  /// Creates a preference storing an optional array of user defaults supported value types
  public convenience init<V>(
    key: String,
    default: ValueType,
    container: UserDefaults = Preferences.defaultContainer
  ) where V: UserDefaultsEncodable, ValueType == [V]? {
    self.init(key: key, defaultValue: `default`, container: container)
  }
  /// Creates a preference storing an optional dictionary of user defaults supported value types
  public convenience init<K, V>(
    key: String,
    default: ValueType,
    container: UserDefaults = Preferences.defaultContainer
  ) where K: StringProtocol, V: UserDefaultsEncodable, ValueType == [K: V]? {
    self.init(key: key, defaultValue: `default`, container: container)
  }
}

extension Preferences.Option {
  /// Creates a preference storing a raw representable where the raw value is a user defaults supported value type
  public convenience init(
    key: String,
    default: ValueType,
    container: UserDefaults = Preferences.defaultContainer
  ) where ValueType: RawRepresentable, ValueType.RawValue: UserDefaultsEncodable {
    let initialValue: ValueType = {
      if let rawValue = (container.value(forKey: key) as? ValueType.RawValue) {
        if let value = ValueType(rawValue: rawValue) {
          return value
        } else {
          Logger.module.error(
            "Failed to load enum preference \"\(key)\" with raw value \(String(describing: rawValue))"
          )
        }
      }
      return `default`
    }()
    self.init(
      key: key,
      initialValue: initialValue,
      defaultValue: `default`,
      container: container,
      writePreferenceValue: { $0.setValue($2.rawValue, forKey: $1) }
    )
  }

  /// Creates a preference storing an optional raw representable where the raw value is a user defaults
  /// supported value type
  public convenience init<R>(
    key: String,
    default: ValueType,
    container: UserDefaults = Preferences.defaultContainer
  ) where ValueType == R?, R: RawRepresentable, R.RawValue: UserDefaultsEncodable {
    let initialValue: R? = {
      if let rawValue = (container.value(forKey: key) as? R.RawValue) {
        return R(rawValue: rawValue)
      }
      return nil
    }()
    self.init(
      key: key,
      initialValue: initialValue,
      defaultValue: `default`,
      container: container,
      writePreferenceValue: { container, key, value in
        if let value = value {
          container.setValue(value.rawValue, forKey: key)
        } else {
          container.removeObject(forKey: key)
        }
      }
    )
  }
}

/// An empty protocol simply here to force the developer to use a user defaults encodable value via generic constraint.
/// DO NOT ADD CONFORMANCE TO ANY OTHER TYPES. These are specifically the types supported by UserDefaults
public protocol UserDefaultsEncodable: Equatable {}
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
extension Dictionary: UserDefaultsEncodable
where Key: StringProtocol, Value: UserDefaultsEncodable {}
