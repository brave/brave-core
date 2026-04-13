// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Preferences
import Web
import XCTest

@testable import Brave

private let deprecatedLastLaunchInfoPrefKey = "dau.last-launch-info"

private func utcTimestamp(year: Int, month: Int, day: Int) -> Int {
  var components = DateComponents(
    timeZone: TimeZone(identifier: "UTC")!,
    year: year,
    month: month,
    day: day
  )
  var calendar = Calendar(identifier: .gregorian)
  calendar.timeZone = TimeZone(identifier: "UTC")!
  return Int(calendar.date(from: components)!.timeIntervalSince1970)
}

class BraveLocalStateMigrationTests: XCTestCase {

  private var localState: FakePrefService!
  private var migration: BraveLocalStateMigration!

  override func setUp() {
    super.setUp()
    Preferences.defaultContainer.removeObject(
      forKey: deprecatedLastLaunchInfoPrefKey
    )
    Preferences.defaultContainer.removeObject(forKey: Preferences.DAU.firstPingParam.key)
    localState = FakePrefService()
    migration = BraveLocalStateMigration(localState: localState)
  }

  override func tearDown() {
    Preferences.defaultContainer.removeObject(
      forKey: deprecatedLastLaunchInfoPrefKey
    )
    Preferences.defaultContainer.removeObject(forKey: Preferences.DAU.firstPingParam.key)
    super.tearDown()
  }

  // MARK: - migrateDAULastLaunchInfoPreference

  func testMigratingLastLaunchInfoWithValidTimestampSetsLastPingDate() {
    let timestamp = utcTimestamp(year: 2026, month: 1, day: 2)
    Preferences.defaultContainer.set(
      [timestamp, 1, 2, 1],
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()

    XCTAssertEqual(localState.string(forPath: kLastCheckYMDPrefName), "2026-01-02")
  }

  func testMigratingLastLaunchInfoWithSingleElementArraySetsLastPingDate() {
    let timestamp = utcTimestamp(year: 2026, month: 1, day: 2)
    Preferences.defaultContainer.set(
      [timestamp],
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()

    XCTAssertEqual(localState.string(forPath: kLastCheckYMDPrefName), "2026-01-02")
  }

  func testMigratingLastLaunchInfoWithEpochTimestampSetsEpochDate() {
    Preferences.defaultContainer.set(
      [0],
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()

    XCTAssertEqual(localState.string(forPath: kLastCheckYMDPrefName), "1970-01-01")
  }

  func testMigratingLastLaunchInfoWithNonNilValueOverwritesFirstPingStatus() {
    Preferences.DAU.firstPingParam.value = true
    Preferences.defaultContainer.removeObject(forKey: Preferences.DAU.firstPingParam.key)
    Preferences.defaultContainer.set(
      [utcTimestamp(year: 2026, month: 1, day: 2)],
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()

    XCTAssertFalse(Preferences.DAU.firstPingParam.value)
  }

  func testMigratingLastLaunchInfoWithNonNilValueClearsLegacyPreferenceKey() {
    Preferences.defaultContainer.set(
      [utcTimestamp(year: 2026, month: 1, day: 2)],
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()

    XCTAssertNil(
      Preferences.defaultContainer.object(
        forKey: deprecatedLastLaunchInfoPrefKey
      )
    )
  }

  func testMigratingLastLaunchInfoWithEmptyArrayDoesNotSetLastPingDate() {
    Preferences.defaultContainer.set(
      [Int](),
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()

    XCTAssertFalse(localState.hasPref(forPath: kLastCheckYMDPrefName))
  }

  func testMigratingLastLaunchInfoWithEmptyArrayDoesNotOverwriteFirstPingStatus() {
    Preferences.DAU.firstPingParam.value = true
    Preferences.defaultContainer.removeObject(forKey: Preferences.DAU.firstPingParam.key)
    Preferences.defaultContainer.set(
      [Int](),
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()

    XCTAssertTrue(Preferences.DAU.firstPingParam.value)
  }

  func testMigratingLastLaunchInfoWithEmptyArrayClearsLegacyPreferenceKey() {
    Preferences.defaultContainer.set(
      [Int](),
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()

    XCTAssertNil(
      Preferences.defaultContainer.object(
        forKey: deprecatedLastLaunchInfoPrefKey
      )
    )
  }

  func testMigratingLastLaunchInfoWithIncompatibleStoredTypeClearsLegacyKeyWithoutSideEffects() {
    Preferences.DAU.firstPingParam.value = true
    Preferences.defaultContainer.removeObject(forKey: Preferences.DAU.firstPingParam.key)
    Preferences.defaultContainer.set(
      "unexpected-string-value",
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()

    XCTAssertNil(
      Preferences.defaultContainer.object(
        forKey: deprecatedLastLaunchInfoPrefKey
      )
    )
    XCTAssertFalse(localState.hasPref(forPath: kLastCheckYMDPrefName))
    XCTAssertTrue(Preferences.DAU.firstPingParam.value)
  }

  func testMigratingLastLaunchInfoWithNoLegacyPreferenceDoesNotSetLocalState() {
    migration.launchMigrations()

    XCTAssertFalse(localState.hasPref(forPath: kLastCheckYMDPrefName))
  }

  func testMigratingLastLaunchInfoWhenPingStatusAlreadyMigratedDoesNotOverwriteFistPingStatus() {
    Preferences.DAU.firstPingParam.value = true
    Preferences.defaultContainer.set(
      [utcTimestamp(year: 2026, month: 1, day: 2)],
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()

    XCTAssertTrue(Preferences.DAU.firstPingParam.value)
  }

  func testMigratingLastLaunchInfoTwiceDoesNotOverwriteLastPingDate() {
    Preferences.defaultContainer.set(
      [utcTimestamp(year: 2026, month: 1, day: 2)],
      forKey: deprecatedLastLaunchInfoPrefKey
    )

    migration.launchMigrations()
    let ymdAfterFirstCall = localState.string(forPath: kLastCheckYMDPrefName)

    migration.launchMigrations()

    XCTAssertEqual(localState.string(forPath: kLastCheckYMDPrefName), ymdAfterFirstCall)
  }
}
