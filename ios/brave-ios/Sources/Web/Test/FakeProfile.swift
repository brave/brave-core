// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

/// A implementation of Profile that can be used in unit tests or SwiftUI Previews.
public class FakeProfile: Profile {
  public let name: String
  public let prefs: any PrefService
  public private(set) var isOffTheRecord: Bool
  public var originalProfile: any Profile {
    if isOffTheRecord, let _originalProfile {
      return _originalProfile
    }
    return self
  }
  public var offTheRecordProfile: any Profile {
    if isOffTheRecord {
      return self
    }
    let otrProfile = FakeProfile(name: "\(name)-OTR")
    otrProfile.isOffTheRecord = true
    otrProfile._originalProfile = self
    return otrProfile
  }

  private var _originalProfile: (any Profile)?

  public init(name: String = "Default", prefs: any PrefService = FakePrefService()) {
    self.name = name
    self.prefs = prefs
    self.isOffTheRecord = false
  }
}

/// A implementation of PrefService that can be used in unit tests or SwiftUI Previews.
public class FakePrefService: PrefService {
  private var prefs: [AnyHashable: Any] = [:]

  public init() {
  }

  public func commitPendingWrite() {
  }

  public func isManagedPreference(forPath path: String) -> Bool {
    return false
  }

  public func boolean(forPath path: String) -> Bool {
    return prefs[path] as? Bool ?? false
  }

  public func integer(forPath path: String) -> Int {
    return prefs[path] as? Int ?? 0
  }

  public func double(forPath path: String) -> Double {
    return prefs[path] as? Double ?? 0.0
  }

  public func string(forPath path: String) -> String {
    return prefs[path] as? String ?? ""
  }

  public func filePath(forPath path: String) -> String {
    return prefs[path] as? String ?? ""
  }

  public func value(forPath path: String) -> BaseValue {
    return prefs[path] as? BaseValue ?? .init()
  }

  public func dictionary(forPath path: String) -> [String: BaseValue] {
    return prefs[path] as? [String: BaseValue] ?? [:]
  }

  public func list(forPath path: String) -> [BaseValue] {
    return prefs[path] as? [BaseValue] ?? []
  }

  public func int64(forPath path: String) -> Int64 {
    return prefs[path] as? Int64 ?? 0
  }

  public func uint64(forPath path: String) -> UInt64 {
    return prefs[path] as? UInt64 ?? 0
  }

  public func timeDelta(forPath path: String) -> TimeInterval {
    return prefs[path] as? TimeInterval ?? 0
  }

  public func time(forPath path: String) -> Date {
    return prefs[path] as? Date ?? Date(timeIntervalSince1970: 0)
  }

  public func userPrefValue(forPath path: String) -> BaseValue? {
    return prefs[path] as? BaseValue
  }

  public func hasPref(forPath path: String) -> Bool {
    return prefs[path] != nil
  }

  public func set(_ value: Bool, forPath path: String) {
    prefs[path] = value
  }
  public func set(_ value: Int, forPath path: String) {
    prefs[path] = value
  }
  public func set(_ value: Double, forPath path: String) {
    prefs[path] = value
  }
  public func set(_ value: String, forPath path: String) {
    prefs[path] = value
  }
  public func set(_ value: BaseValue, forPath path: String) {
    prefs[path] = value
  }
  public func set(_ dict: [String: BaseValue], forPath path: String) {
    prefs[path] = dict
  }
  public func set(_ list: [BaseValue], forPath path: String) {
    prefs[path] = list
  }
  public func set(filePath: String, forPath path: String) {
    prefs[path] = filePath
  }
  public func set(timeDelta: TimeInterval, forPath: String) {
    prefs[forPath] = timeDelta
  }
  public func set(_ value: Int64, forPath path: String) {
    prefs[path] = value
  }
  public func set(_ value: UInt64, forPath path: String) {
    prefs[path] = value
  }
  public func set(_ time: Date, forPath path: String) {
    prefs[path] = time
  }
  public func clearPref(forPath path: String) {
    prefs[path] = nil
  }
}
