// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import XCTest

@testable import BraveTalk

class BraveTalkTests: XCTestCase {
  @MainActor func testTalkEnabledByDefault() {
    let prefService = MockBraveTalkPrefService(_isManagedPreference: false, _boolean: false)
    XCTAssertTrue(prefService.isBraveTalkAvailable)
    XCTAssertTrue(BraveTalkJitsiCoordinator.isIntegrationEnabled(for: prefService))
  }

  @MainActor func testTalkDisabledByPolicy() {
    let prefService = MockBraveTalkPrefService(_isManagedPreference: true, _boolean: true)
    XCTAssertFalse(prefService.isBraveTalkAvailable)
    XCTAssertFalse(BraveTalkJitsiCoordinator.isIntegrationEnabled(for: prefService))
  }

  @MainActor func testTalkEnabledByPolicy() {
    let prefService = MockBraveTalkPrefService(_isManagedPreference: true, _boolean: false)
    XCTAssertTrue(prefService.isBraveTalkAvailable)
    XCTAssertTrue(BraveTalkJitsiCoordinator.isIntegrationEnabled(for: prefService))
  }
}

private class MockBraveTalkPrefService: PrefService {
  var _isManagedPreference: () -> Bool
  var _boolean: () -> Bool

  init(
    _isManagedPreference: @autoclosure @escaping () -> Bool,
    _boolean: @autoclosure @escaping () -> Bool
  ) {
    self._isManagedPreference = _isManagedPreference
    self._boolean = _boolean
  }

  func commitPendingWrite() {
  }

  func isManagedPreference(forPath path: String) -> Bool {
    _isManagedPreference()
  }

  func boolean(forPath path: String) -> Bool {
    _boolean()
  }

  func integer(forPath path: String) -> Int {
    return 0
  }

  func double(forPath path: String) -> Double {
    return 0.0
  }

  func string(forPath path: String) -> String {
    return ""
  }

  func filePath(forPath path: String) -> String {
    return ""
  }

  func value(forPath path: String) -> BaseValue {
    return BaseValue()
  }

  func dictionary(forPath path: String) -> [String: BaseValue] {
    return [:]
  }

  func list(forPath path: String) -> [BaseValue] {
    return []
  }

  func int64(forPath path: String) -> Int64 {
    return 0
  }

  func uint64(forPath path: String) -> UInt64 {
    return 0
  }

  func timeDelta(forPath path: String) -> TimeInterval {
    return 0
  }

  func time(forPath path: String) -> Date {
    return Date()
  }

  func userPrefValue(forPath path: String) -> BaseValue? {
    return nil
  }

  func hasPref(forPath path: String) -> Bool {
    return true
  }

  func set(_ value: Bool, forPath path: String) {}
  func set(_ value: Int, forPath path: String) {}
  func set(_ value: Double, forPath path: String) {}
  func set(_ value: String, forPath path: String) {}
  func set(_ value: BaseValue, forPath path: String) {}
  func set(_ dict: [String: BaseValue], forPath path: String) {}
  func set(_ list: [BaseValue], forPath path: String) {}
  func set(filePath: String, forPath path: String) {}
  func set(timeDelta: TimeInterval, forPath: String) {}
  func set(_ value: Int64, forPath path: String) {}
  func set(_ value: UInt64, forPath path: String) {}
  func set(_ time: Date, forPath path: String) {}
  func clearPref(forPath path: String) {}
}
