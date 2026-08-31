// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import XCTest

@testable import Brave

private let backgroundSponsoredImagesKey = "newtabpage.background-sponsored-images"
private let backgroundMediaTypeRawKey = "newtabpage.background-media-type"

class MigrationTests: XCTestCase {

  override func setUp() {
    super.setUp()
    Preferences.Migration.backgroundSponsoredImagesCompleted.reset()
    Preferences.Migration.sponsoredAdsEnabledMigrationCompleted.reset()
    Preferences.BraveAds.sponsoredEnabled.reset()
    Preferences.defaultContainer.removeObject(forKey: backgroundSponsoredImagesKey)
    Preferences.defaultContainer.removeObject(forKey: backgroundMediaTypeRawKey)
  }

  override func tearDown() {
    Preferences.Migration.backgroundSponsoredImagesCompleted.reset()
    Preferences.Migration.sponsoredAdsEnabledMigrationCompleted.reset()
    Preferences.BraveAds.sponsoredEnabled.reset()
    Preferences.defaultContainer.removeObject(forKey: backgroundSponsoredImagesKey)
    Preferences.defaultContainer.removeObject(forKey: backgroundMediaTypeRawKey)
    super.tearDown()
  }

  // MARK: - migrateBackgroundSponsoredImages

  func testWhenBackgroundSponsoredImagesWasEnabledSponsoredAdsBecomesEnabled() {
    Preferences.defaultContainer.set(true, forKey: backgroundSponsoredImagesKey)

    Preferences.migrateBackgroundSponsoredImages()

    XCTAssertTrue(Preferences.BraveAds.sponsoredEnabled.value)
  }

  func testWhenBackgroundSponsoredImagesWasDisabledSponsoredAdsBecomesDisabled() {
    Preferences.defaultContainer.set(false, forKey: backgroundSponsoredImagesKey)

    Preferences.migrateBackgroundSponsoredImages()

    XCTAssertFalse(Preferences.BraveAds.sponsoredEnabled.value)
  }

  func testWhenBackgroundSponsoredImagesPreferenceIsAbsentSponsoredAdsEnabledStaysUnchanged() {
    Preferences.migrateBackgroundSponsoredImages()

    XCTAssertTrue(Preferences.BraveAds.sponsoredEnabled.value)
  }

  func testMigratingBackgroundSponsoredImagesRemovesTheOldPreferenceKey() {
    Preferences.defaultContainer.set(true, forKey: backgroundSponsoredImagesKey)

    Preferences.migrateBackgroundSponsoredImages()

    XCTAssertNil(Preferences.defaultContainer.object(forKey: backgroundSponsoredImagesKey))
  }

  func testMigratingBackgroundSponsoredImagesMarksTheMigrationAsCompleted() {
    Preferences.migrateBackgroundSponsoredImages()

    XCTAssertTrue(Preferences.Migration.backgroundSponsoredImagesCompleted.value)
  }

  func testWhenBackgroundMediaTypeIsDefaultImagesOnlySponsoredAdsBecomesDisabled() {
    Preferences.defaultContainer.set(0, forKey: backgroundMediaTypeRawKey)

    Preferences.migrateSponsoredAdsEnabled()

    XCTAssertFalse(Preferences.BraveAds.sponsoredEnabled.value)
  }

  func testWhenBackgroundMediaTypeIsSponsoredImagesSponsoredAdsBecomesEnabled() {
    Preferences.defaultContainer.set(1, forKey: backgroundMediaTypeRawKey)

    Preferences.migrateSponsoredAdsEnabled()

    XCTAssertTrue(Preferences.BraveAds.sponsoredEnabled.value)
  }

  func testWhenBackgroundMediaTypeIsSponsoredImagesAndVideosSponsoredAdsBecomesEnabled() {
    Preferences.defaultContainer.set(2, forKey: backgroundMediaTypeRawKey)

    Preferences.migrateSponsoredAdsEnabled()

    XCTAssertTrue(Preferences.BraveAds.sponsoredEnabled.value)
  }

  func testWhenBackgroundMediaTypePreferenceIsAbsentSponsoredAdsBecomesEnabled() {
    Preferences.migrateSponsoredAdsEnabled()

    XCTAssertTrue(Preferences.BraveAds.sponsoredEnabled.value)
  }

  func testMigratingSponsoredAdsEnabledRemovesTheOldPreferenceKey() {
    Preferences.defaultContainer.set(0, forKey: backgroundMediaTypeRawKey)

    Preferences.migrateSponsoredAdsEnabled()

    XCTAssertNil(Preferences.defaultContainer.object(forKey: backgroundMediaTypeRawKey))
  }

  func testMigratingSponsoredAdsEnabledMarksTheMigrationAsCompleted() {
    Preferences.migrateSponsoredAdsEnabled()

    XCTAssertTrue(Preferences.Migration.sponsoredAdsEnabledMigrationCompleted.value)
  }

  func
    testWhenBackgroundMediaTypeValueIsNonNumericTheOldKeyIsRemovedAndSponsoredAdsBecomesEnabled()
  {
    Preferences.defaultContainer.set("unexpected-string-value", forKey: backgroundMediaTypeRawKey)

    Preferences.migrateSponsoredAdsEnabled()

    XCTAssertNil(Preferences.defaultContainer.object(forKey: backgroundMediaTypeRawKey))
    XCTAssertTrue(Preferences.BraveAds.sponsoredEnabled.value)
    XCTAssertTrue(Preferences.Migration.sponsoredAdsEnabledMigrationCompleted.value)
  }

  func
    testWhenBothOldPreferencesArePresentBackgroundMediaTypeTakesPrecedenceOverBackgroundSponsoredImages()
  {
    Preferences.defaultContainer.set(true, forKey: backgroundSponsoredImagesKey)
    Preferences.defaultContainer.set(0, forKey: backgroundMediaTypeRawKey)

    Preferences.migrateBackgroundSponsoredImages()
    Preferences.migrateSponsoredAdsEnabled()

    XCTAssertFalse(Preferences.BraveAds.sponsoredEnabled.value)
  }

  func testWhenBackgroundSponsoredImagesWasAlreadyMigratedBackgroundMediaTypeStillAppliesItsValue()
  {
    Preferences.Migration.backgroundSponsoredImagesCompleted.value = true
    Preferences.defaultContainer.set(0, forKey: backgroundMediaTypeRawKey)

    Preferences.migrateSponsoredAdsEnabled()

    XCTAssertFalse(Preferences.BraveAds.sponsoredEnabled.value)
    XCTAssertTrue(Preferences.Migration.backgroundSponsoredImagesCompleted.value)
  }

  func testRunningTheSponsoredAdsMigrationTwiceDoesNotOverwriteUserChange() {
    Preferences.defaultContainer.set(0, forKey: backgroundMediaTypeRawKey)
    Preferences.migrateSponsoredAdsEnabled()
    XCTAssertFalse(Preferences.BraveAds.sponsoredEnabled.value)

    Preferences.BraveAds.sponsoredEnabled.value = true
    Preferences.defaultContainer.set(0, forKey: backgroundMediaTypeRawKey)

    Preferences.migrateSponsoredAdsEnabled()

    XCTAssertTrue(Preferences.BraveAds.sponsoredEnabled.value)
  }
}
