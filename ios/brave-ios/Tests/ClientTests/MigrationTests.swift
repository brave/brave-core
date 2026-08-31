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

private let backgroundMediaTypeRawKey = "newtabpage.background-media-type"

@MainActor
class MigrationTests: XCTestCase {

  private var prefs: FakePrefService!

  override func setUp() {
    super.setUp()
    Preferences.defaultContainer.removeObject(forKey: backgroundMediaTypeRawKey)
    prefs = FakePrefService()
  }

  override func tearDown() {
    Preferences.defaultContainer.removeObject(forKey: backgroundMediaTypeRawKey)
    super.tearDown()
  }

  // MARK: - migrateSponsoredAdsEnabledPreference

  func testMigratingDefaultImagesOnlyDisablesSponsoredAds() {
    Preferences.defaultContainer.set(0, forKey: backgroundMediaTypeRawKey)

    BraveProfileMigrations.migrateSponsoredAdsEnabledPreference(prefs: prefs)

    XCTAssertFalse(prefs.boolean(forPath: kBraveAdsSponsoredEnabledPrefName))
  }

  func testMigratingSponsoredImagesEnablesSponsoredAds() {
    Preferences.defaultContainer.set(1, forKey: backgroundMediaTypeRawKey)

    BraveProfileMigrations.migrateSponsoredAdsEnabledPreference(prefs: prefs)

    XCTAssertTrue(prefs.boolean(forPath: kBraveAdsSponsoredEnabledPrefName))
  }

  func testMigratingSponsoredImagesAndVideosEnablesSponsoredAds() {
    Preferences.defaultContainer.set(2, forKey: backgroundMediaTypeRawKey)

    BraveProfileMigrations.migrateSponsoredAdsEnabledPreference(prefs: prefs)

    XCTAssertTrue(prefs.boolean(forPath: kBraveAdsSponsoredEnabledPrefName))
  }

  func testMigratingAnUnknownDeprecatedPreferenceEnablesSponsoredAds() {
    Preferences.defaultContainer.set(3, forKey: backgroundMediaTypeRawKey)

    BraveProfileMigrations.migrateSponsoredAdsEnabledPreference(prefs: prefs)

    XCTAssertTrue(prefs.boolean(forPath: kBraveAdsSponsoredEnabledPrefName))
  }

  func testMigratingWithNoDeprecatedPreferenceDoesNotSetSponsoredAdsPref() {
    BraveProfileMigrations.migrateSponsoredAdsEnabledPreference(prefs: prefs)

    XCTAssertFalse(prefs.hasPref(forPath: kBraveAdsSponsoredEnabledPrefName))
  }

  func testMigratingClearsTheDeprecatedPreferenceKey() {
    Preferences.defaultContainer.set(1, forKey: backgroundMediaTypeRawKey)

    BraveProfileMigrations.migrateSponsoredAdsEnabledPreference(prefs: prefs)

    XCTAssertNil(Preferences.defaultContainer.object(forKey: backgroundMediaTypeRawKey))
  }

  func testMigratingTwiceDoesNotOverwriteAnAlreadyMigratedPreference() {
    Preferences.defaultContainer.set(1, forKey: backgroundMediaTypeRawKey)
    BraveProfileMigrations.migrateSponsoredAdsEnabledPreference(prefs: prefs)

    prefs.set(false, forPath: kBraveAdsSponsoredEnabledPrefName)
    BraveProfileMigrations.migrateSponsoredAdsEnabledPreference(prefs: prefs)

    XCTAssertFalse(prefs.boolean(forPath: kBraveAdsSponsoredEnabledPrefName))
  }
}
