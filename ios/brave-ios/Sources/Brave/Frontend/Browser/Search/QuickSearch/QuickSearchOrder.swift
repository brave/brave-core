// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences

extension QuickSearchItem {
  /// Whether the Brave Search verticals are part of the catalog. They ship to new installs only.
  private static var includesBraveSearchVerticals: Bool {
    FeatureList.kBraveSearchVerticalsEnabled.enabled
      && Preferences.BraveSearch.braveSearchVerticalsEligible.value
  }

  /// Whether the stored order has placed the verticals, which only happens once the user has
  /// reordered the row.
  private static var hasStoredBraveSearchVerticals: Bool {
    (Preferences.Search.quickSearchOrder.value ?? [])
      .lazy
      .compactMap(QuickSearchItemID.init(rawValue:))
      .contains(where: \.isBraveVertical)
  }

  /// The items eligible for the quick search row — every ordered engine except `defaultEngine`,
  /// which consumers pin to the front themselves, plus the Brave Search verticals — in the
  /// user's stored quick search order.
  static func quickSearchCatalog(
    from engines: [OpenSearchEngine],
    excludingDefault defaultEngine: OpenSearchEngine?
  ) -> [QuickSearchItem] {
    let defaultEngineID = defaultEngine.map(QuickSearchItemID.init(engine:))
    let engineItems =
      engines
      .filter { QuickSearchItemID(engine: $0) != defaultEngineID }
      .map { QuickSearchItem(source: .engine($0)) }

    guard includesBraveSearchVerticals else { return engineItems.applyingQuickSearchOrder() }

    let verticals = BraveSearchVertical.Kind.allCases.map {
      QuickSearchItem(source: .braveVertical(.init(kind: $0)))
    }

    if hasStoredBraveSearchVerticals {
      return (engineItems + verticals).applyingQuickSearchOrder()
    }

    // The verticals have never been placed, so introduce them directly behind Brave Search
    var items = engineItems.applyingQuickSearchOrder()
    let braveSearchIndex = items.firstIndex { $0.id == .braveSearchEngine }
    items.insert(contentsOf: verticals, at: braveSearchIndex.map { $0 + 1 } ?? 0)
    return items
  }
}

extension Array where Element == QuickSearchItem {
  func applyingQuickSearchOrder() -> [QuickSearchItem] {
    let storedOrder = (Preferences.Search.quickSearchOrder.value ?? [])
      .compactMap(QuickSearchItemID.init(rawValue:))

    var itemsByID = Dictionary(
      map { ($0.id, $0) },
      uniquingKeysWith: { first, _ in first }
    )

    // Stored order first, dropping IDs no longer in the catalog.
    var orderedItems = storedOrder.compactMap { itemsByID.removeValue(forKey: $0) }
    // Then anything the stored order hasn't seen yet, in catalog order.
    orderedItems += filter { itemsByID[$0.id] != nil }
    return orderedItems
  }

  func filteringQuickSearchDisabled() -> [QuickSearchItem] {
    let disabledIDs = Set(
      (Preferences.Search.quickSearchDisabled.value ?? [])
        .compactMap(QuickSearchItemID.init(rawValue:))
    )
    return filter { !disabledIDs.contains($0.id) }
  }

  /// Persists the receiver's order as the quick search order.
  func persistQuickSearchOrder() {
    Preferences.Search.quickSearchOrder.value = map(\.id.rawValue)
  }
}

extension QuickSearchItemID {
  var isQuickSearchEnabled: Bool {
    !(Preferences.Search.quickSearchDisabled.value ?? []).contains(rawValue)
  }

  func setQuickSearchEnabled(_ enabled: Bool) {
    var disabledRawValues = Preferences.Search.quickSearchDisabled.value ?? []
    disabledRawValues.removeAll { $0 == rawValue }
    if !enabled {
      disabledRawValues.append(rawValue)
    }
    Preferences.Search.quickSearchDisabled.value = disabledRawValues
  }

  /// Gives the receiver the first stored position, so a freshly added engine shows up next to
  /// the default engine rather than at the end of the row.
  func moveToFrontOfQuickSearchOrder() {
    guard let order = Preferences.Search.quickSearchOrder.value else { return }
    Preferences.Search.quickSearchOrder.value = [rawValue] + order.filter { $0 != rawValue }
  }

  /// Moves persisted quick search state from the receiver to `newID`, keeping its position.
  func rekeyQuickSearchState(to newID: QuickSearchItemID) {
    guard newID != self else { return }

    if let order = Preferences.Search.quickSearchOrder.value {
      Preferences.Search.quickSearchOrder.value = order.map {
        $0 == rawValue ? newID.rawValue : $0
      }
    }

    if !isQuickSearchEnabled {
      setQuickSearchEnabled(true)
      newID.setQuickSearchEnabled(false)
    }
  }

  /// Removes all persisted quick search state for the receiver.
  func removeQuickSearchState() {
    if let order = Preferences.Search.quickSearchOrder.value {
      Preferences.Search.quickSearchOrder.value = order.filter { $0 != rawValue }
    }
    if let disabled = Preferences.Search.quickSearchDisabled.value {
      Preferences.Search.quickSearchDisabled.value = disabled.filter { $0 != rawValue }
    }
  }
}
