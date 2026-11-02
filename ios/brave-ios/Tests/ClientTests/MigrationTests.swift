// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import XCTest

@testable import Brave

@MainActor
class MigrationTests: XCTestCase {

  // MARK: - isSponsoredAdsEnabled(forBackgroundMediaTypeRawValue:)

  func testDefaultImagesOnlyDisablesSponsoredAds() {
    XCTAssertFalse(
      BraveProfileMigrations.isSponsoredAdsEnabled(forBackgroundMediaTypeRawValue: 0)
    )
  }

  func testSponsoredImagesEnablesSponsoredAds() {
    XCTAssertTrue(
      BraveProfileMigrations.isSponsoredAdsEnabled(forBackgroundMediaTypeRawValue: 1)
    )
  }

  func testDeprecatedSponsoredImagesAndVideosEnablesSponsoredAds() {
    XCTAssertTrue(
      BraveProfileMigrations.isSponsoredAdsEnabled(forBackgroundMediaTypeRawValue: 2)
    )
  }
}

/// The legacy `newtabpage.topsites-mode` raw values, as written by each release.
///
/// 1.99.x inserted `none` at 0 and shifted the rest, so the same integer means different things
/// depending on which release stored it. Spelled out here so that the expectations below describe
/// what a device actually has on disk.
private enum LegacyRawValue {
  /// Seeded at launch by 1.97.x, and written by 1.98.x for a mode neither could render.
  static let mostVisitedUpTo198 = 0
  static let favouriteUpTo198 = 1
  static let noneUpTo198 = 2

  static let noneFrom199 = 0
  static let mostVisitedFrom199 = 1
  static let favouriteFrom199 = 2
}

@MainActor
class TopSitesModeMigrationTests: XCTestCase {

  private var container: UserDefaults { Preferences.defaultContainer }
  private var key: String { Preferences.NewTabPage.topSitesModeV2.key }
  private var legacyKey: String { Preferences.NewTabPage.topSitesMode.key }

  override func setUp() {
    super.setUp()
    resetPreferences()
  }

  override func tearDown() {
    resetPreferences()
    super.tearDown()
  }

  /// Clears the stored values and the in-memory values of the long lived `Option` singletons.
  private func resetPreferences() {
    Preferences.NewTabPage.topSitesModeV2.reset()
    Preferences.NewTabPage.topSitesMode.value = nil
    container.removeObject(forKey: key)
    container.removeObject(forKey: legacyKey)
  }

  // MARK: - Stored representation

  /// Pins the stored representation. This bug was a reordering of integer raw values silently
  /// remapping preferences already on disk, so editing a case must fail loudly instead.
  func testRawValuesAreStable() {
    XCTAssertEqual(TopSitesMode.none.rawValue, "none")
    XCTAssertEqual(TopSitesMode.mostVisited.rawValue, "most-visited")
    XCTAssertEqual(TopSitesMode.favourite.rawValue, "favourite")
  }

  func testStoresAndReadsBackStringRawValues() {
    Preferences.NewTabPage.topSitesModeV2.value = .mostVisited

    XCTAssertEqual(container.string(forKey: key), "most-visited")
    XCTAssertEqual(
      Preferences.Option<TopSitesMode>(key: key, default: TopSitesMode.none).value,
      .mostVisited
    )
  }

  func testUnrecognizedStoredValueFallsBackToTheDefault() {
    container.setValue("random_value", forKey: key)

    XCTAssertEqual(
      Preferences.Option<TopSitesMode>(key: key, default: .favourite).value,
      .favourite
    )
  }

  /// The migration skips devices that already have a value, which it cannot determine with
  /// `isValueStored` because the container holds the raw value rather than a `TopSitesMode`.
  func testIsRawValueStoredReflectsTheContainer() {
    XCTAssertFalse(Preferences.NewTabPage.topSitesModeV2.isRawValueStored)

    Preferences.NewTabPage.topSitesModeV2.value = TopSitesMode.none

    XCTAssertTrue(Preferences.NewTabPage.topSitesModeV2.isRawValueStored)
  }

  // MARK: - topSitesMode(forLegacyRawValue:showNewTabFavourites:)

  /// The reported bug: favourites on 1.97.x, then an upgrade forced most visited sites.
  func testLegacyFavouriteMapsToFavourites() {
    XCTAssertEqual(
      BraveProfileMigrations.topSitesMode(
        forLegacyRawValue: LegacyRawValue.favouriteUpTo198,
        showNewTabFavourites: nil
      ),
      .favourite
    )
  }

  /// A fresh 1.97.x install seeded `mostVisited`, because launch happens before any favourite can
  /// exist, and adding favourites afterwards never reseeded it. No release that wrote this value
  /// could render most visited sites, so favourites is what the user was looking at.
  func testLegacyMostVisitedMapsToFavourites() {
    XCTAssertEqual(
      BraveProfileMigrations.topSitesMode(
        forLegacyRawValue: LegacyRawValue.mostVisitedUpTo198,
        showNewTabFavourites: nil
      ),
      .favourite
    )
  }

  func testLegacyNoneMapsToNone() {
    XCTAssertEqual(
      BraveProfileMigrations.topSitesMode(
        forLegacyRawValue: LegacyRawValue.noneUpTo198,
        showNewTabFavourites: nil
      ),
      TopSitesMode.none
    )
  }

  func testUnrecognizedLegacyRawValueMapsToFavourites() {
    XCTAssertEqual(
      BraveProfileMigrations.topSitesMode(forLegacyRawValue: 7, showNewTabFavourites: nil),
      .favourite
    )
  }

  /// The flag wins: 1.97.x is the only release where it was settable, and the mode there was an
  /// unread launch seed. Reading the seed instead discarded an explicit opt out.
  func testShowNewTabFavouritesTakesPrecedenceOverLegacyMode() {
    XCTAssertEqual(
      BraveProfileMigrations.topSitesMode(
        forLegacyRawValue: LegacyRawValue.mostVisitedUpTo198,
        showNewTabFavourites: false
      ),
      TopSitesMode.none
    )
  }

  func testShowNewTabFavouritesEnabledMapsToFavourites() {
    XCTAssertEqual(
      BraveProfileMigrations.topSitesMode(forLegacyRawValue: nil, showNewTabFavourites: true),
      .favourite
    )
  }

  func testShowNewTabFavouritesDisabledMapsToNone() {
    XCTAssertEqual(
      BraveProfileMigrations.topSitesMode(forLegacyRawValue: nil, showNewTabFavourites: false),
      TopSitesMode.none
    )
  }

  /// `showNewTabFavourites` defaults to `true`, so an untouched toggle must not be read as an opt
  /// out. Treating a default as a stored value is what made the previous migration a no-op for
  /// everyone who never flipped that switch.
  func testNothingStoredMapsToNothing() {
    XCTAssertNil(
      BraveProfileMigrations.topSitesMode(forLegacyRawValue: nil, showNewTabFavourites: nil)
    )
  }

  // MARK: - 1.99.x and later testers

  /// Documents the accepted cost of the ambiguous raw value: `2` is read as the 1.98.x `none`,
  /// which resets a tester who picked favourites on 1.99.x. Honouring an explicit opt out on the
  /// far larger release channel is the safer side to err on.
  func testTesterWhoPickedFavouritesIsResetToNone() {
    XCTAssertEqual(
      BraveProfileMigrations.topSitesMode(
        forLegacyRawValue: LegacyRawValue.favouriteFrom199,
        showNewTabFavourites: nil
      ),
      TopSitesMode.none
    )
  }

  func testTesterWhoPickedMostVisitedIsResetToFavourites() {
    XCTAssertEqual(
      BraveProfileMigrations.topSitesMode(
        forLegacyRawValue: LegacyRawValue.mostVisitedFrom199,
        showNewTabFavourites: nil
      ),
      .favourite
    )
  }

  func testTesterWhoTurnedTopSitesOffGetsThemBack() {
    XCTAssertEqual(
      BraveProfileMigrations.topSitesMode(
        forLegacyRawValue: LegacyRawValue.noneFrom199,
        showNewTabFavourites: nil
      ),
      .favourite
    )
  }
}
