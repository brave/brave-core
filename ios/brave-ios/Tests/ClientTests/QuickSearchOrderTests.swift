// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import SwiftUI
import UIKit
import XCTest

@testable import Brave

class QuickSearchOrderTests: XCTestCase {

  override func setUp() {
    super.setUp()

    Preferences.Search.orderedEngines.reset()
    Preferences.Search.disabledEngines.reset()
    Preferences.Search.quickSearchOrder.reset()
    Preferences.Search.quickSearchDisabled.reset()
  }

  // MARK: - Helpers

  private func makeItem(engineID: String) -> QuickSearchItem {
    QuickSearchItem(
      source: .engine(
        OpenSearchEngine(
          engineID: engineID,
          shortName: engineID.capitalized,
          image: UIImage(),
          searchTemplate: "https://\(engineID)/?q={searchTerms}",
          suggestTemplate: nil,
          isCustomEngine: false
        )
      )
    )
  }

  private func makeVerticalItem(id: String) -> QuickSearchItem {
    QuickSearchItem(
      source: .braveVertical(
        BraveSearchVertical(id: id, accessibilityTitle: id, icon: Image(systemName: "photo"))
      )
    )
  }

  private func setStoredOrder(_ ids: [QuickSearchItemID]) {
    Preferences.Search.quickSearchOrder.value = ids.map(\.rawValue)
  }

  private func setDisabled(_ ids: [QuickSearchItemID]) {
    Preferences.Search.quickSearchDisabled.value = ids.map(\.rawValue)
  }

  // MARK: - applyingQuickSearchOrder

  func testReturnsCatalogOrderWhenPrefIsNil() {
    let catalog = [makeItem(engineID: "google"), makeItem(engineID: "bing")]
    XCTAssertNil(Preferences.Search.quickSearchOrder.value)

    XCTAssertEqual(
      catalog.applyingQuickSearchOrder().map(\.id),
      [.engine("google"), .engine("bing")]
    )
  }

  func testAppliesStoredOrder() {
    let catalog = [
      makeItem(engineID: "google"),
      makeItem(engineID: "bing"),
      makeItem(engineID: "ddg"),
    ]
    setStoredOrder([.engine("ddg"), .engine("google"), .engine("bing")])

    XCTAssertEqual(
      catalog.applyingQuickSearchOrder().map(\.id),
      [.engine("ddg"), .engine("google"), .engine("bing")]
    )
  }

  /// A newly added custom engine has no stored position: it must appear, at the end.
  func testAppendsItemsMissingFromStoredOrder() {
    let catalog = [
      makeItem(engineID: "google"),
      makeItem(engineID: "bing"),
      makeItem(engineID: "ddg"),
    ]
    setStoredOrder([.engine("bing")])

    XCTAssertEqual(
      catalog.applyingQuickSearchOrder().map(\.id),
      [.engine("bing"), .engine("google"), .engine("ddg")]
    )
  }

  /// Bundled engines come from `InitialSearchEngines` and vary by locale, so a stored ID
  /// can outlive its catalog entry without anything having been deleted.
  func testDropsStoredIDsNoLongerInCatalog() {
    let catalog = [makeItem(engineID: "google")]
    setStoredOrder([.engine("yandex"), .engine("google")])

    XCTAssertEqual(catalog.applyingQuickSearchOrder().map(\.id), [.engine("google")])
  }

  func testApplyingOrderDoesNotWritePrefs() {
    _ = [makeItem(engineID: "google")].applyingQuickSearchOrder()
    XCTAssertNil(Preferences.Search.quickSearchOrder.value)
  }

  // MARK: - filteringQuickSearchDisabled

  func testKeepsEverythingWhenDisabledPrefIsNil() {
    let catalog = [makeItem(engineID: "google"), makeItem(engineID: "bing")]
    XCTAssertEqual(catalog.filteringQuickSearchDisabled().count, 2)
  }

  func testFiltersDisabledItems() {
    let catalog = [
      makeItem(engineID: "google"),
      makeItem(engineID: "bing"),
      makeVerticalItem(id: "images"),
    ]
    setDisabled([.engine("bing"), .braveVertical("images")])

    XCTAssertEqual(catalog.filteringQuickSearchDisabled().map(\.id), [.engine("google")])
  }

  // MARK: - persistQuickSearchOrder

  func testPersistThenApplyRoundTrips() {
    let catalog = [
      makeItem(engineID: "google"),
      makeItem(engineID: "bing"),
      makeItem(engineID: "ddg"),
    ]
    let reordered = [catalog[2], catalog[0], catalog[1]]
    reordered.persistQuickSearchOrder()

    XCTAssertEqual(
      Preferences.Search.quickSearchOrder.value,
      ["engine:ddg", "engine:google", "engine:bing"]
    )
    XCTAssertEqual(
      catalog.applyingQuickSearchOrder().map(\.id),
      reordered.map(\.id)
    )
  }

  // MARK: - isQuickSearchEnabled / setQuickSearchEnabled

  func testItemsAreEnabledByDefault() {
    XCTAssertTrue(QuickSearchItemID.engine("google").isQuickSearchEnabled)
  }

  func testDisableThenEnable() {
    let id = QuickSearchItemID.engine("google")

    id.setQuickSearchEnabled(false)
    XCTAssertFalse(id.isQuickSearchEnabled)
    XCTAssertEqual(Preferences.Search.quickSearchDisabled.value, ["engine:google"])

    id.setQuickSearchEnabled(true)
    XCTAssertTrue(id.isQuickSearchEnabled)
    XCTAssertEqual(Preferences.Search.quickSearchDisabled.value, [])
  }

  func testDisablingIsIdempotent() {
    let id = QuickSearchItemID.engine("google")
    id.setQuickSearchEnabled(false)
    id.setQuickSearchEnabled(false)

    XCTAssertEqual(Preferences.Search.quickSearchDisabled.value, ["engine:google"])
  }

  func testSetEnabledLeavesOrderUntouched() {
    QuickSearchItemID.engine("google").setQuickSearchEnabled(false)
    XCTAssertNil(Preferences.Search.quickSearchOrder.value)
  }

  // MARK: - moveToFrontOfQuickSearchOrder

  func testMoveToFrontOfOrder() {
    let id = QuickSearchItemID.customEngine("https://new/?q=%s")
    setStoredOrder([.engine("google"), .engine("bing")])

    id.moveToFrontOfQuickSearchOrder()

    XCTAssertEqual(
      Preferences.Search.quickSearchOrder.value,
      [id.rawValue, "engine:google", "engine:bing"]
    )
  }

  func testMoveToFrontOfOrderDoesNotDuplicate() {
    let id = QuickSearchItemID.engine("bing")
    setStoredOrder([.engine("google"), id])

    id.moveToFrontOfQuickSearchOrder()

    XCTAssertEqual(Preferences.Search.quickSearchOrder.value, ["engine:bing", "engine:google"])
  }

  // MARK: - rekeyQuickSearchState

  // user might edit his custom engine which their id is seeded from their input
  func testRekeyKeepsPositionInOrder() {
    let old = QuickSearchItemID.customEngine("https://old/?q=%s")
    let new = QuickSearchItemID.customEngine("https://new/?q=%s")
    setStoredOrder([.engine("google"), old, .engine("bing")])

    old.rekeyQuickSearchState(to: new)

    XCTAssertEqual(
      Preferences.Search.quickSearchOrder.value,
      ["engine:google", new.rawValue, "engine:bing"]
    )
  }

  func testRekeyCarriesDisabledState() {
    let old = QuickSearchItemID.customEngine("https://old/?q=%s")
    let new = QuickSearchItemID.customEngine("https://new/?q=%s")
    setStoredOrder([old])
    setDisabled([old, .engine("bing")])

    old.rekeyQuickSearchState(to: new)

    XCTAssertTrue(old.isQuickSearchEnabled)
    XCTAssertFalse(new.isQuickSearchEnabled)
    XCTAssertEqual(
      Preferences.Search.quickSearchDisabled.value,
      ["engine:bing", "custom:https://new/?q=%s"]
    )
  }

  func testRekeyLeavesDisabledAloneWhenItemWasEnabled() {
    let old = QuickSearchItemID.customEngine("https://old/?q=%s")
    setStoredOrder([old])
    setDisabled([.engine("bing")])

    old.rekeyQuickSearchState(to: .customEngine("https://new/?q=%s"))

    XCTAssertEqual(Preferences.Search.quickSearchDisabled.value, ["engine:bing"])
  }

  func testRekeyToSameWontDuplicate() {
    let id = QuickSearchItemID.engine("google")
    setStoredOrder([id])

    id.rekeyQuickSearchState(to: id)

    XCTAssertEqual(Preferences.Search.quickSearchOrder.value, ["engine:google"])
  }

  // MARK: - removeQuickSearchState

  func testRemoveDropsOrderAndDisabledEntries() {
    let id = QuickSearchItemID.customEngine("https://old/?q=%s")
    setStoredOrder([.engine("google"), id, .engine("bing")])
    setDisabled([id, .engine("bing")])

    id.removeQuickSearchState()

    XCTAssertEqual(
      Preferences.Search.quickSearchOrder.value,
      ["engine:google", "engine:bing"]
    )
    XCTAssertEqual(Preferences.Search.quickSearchDisabled.value, ["engine:bing"])
  }
}
