// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Data
import Foundation
import Preferences
import Shared
import os.log

/// A single tile displayed in the top sites UI.
struct TopSiteTile: Identifiable, Hashable {
  /// Identifies a tile across updates. Used by consumers backed by a diffable data source.
  enum ID: Hashable {
    case favorite(NSManagedObjectID)
    case mostVisited(URL)
  }

  let id: ID
  let url: URL
  let title: String?

  /// Favorites are URLs the user defined, which affects how a navigation to them is treated.
  var isFavorite: Bool {
    if case .favorite = id { return true }
    return false
  }
}

extension TopSiteTile {
  /// A favorite without a usable URL cannot be rendered or opened, so it isn't a tile.
  init?(_ favorite: Favorite) {
    guard let url = favorite.url?.asURL else { return nil }
    self.init(
      id: .favorite(favorite.objectID),
      url: url,
      title: favorite.displayTitle ?? favorite.url
    )
  }

  init(_ tile: NTPTile) {
    self.init(id: .mostVisited(tile.url as URL), url: tile.url as URL, title: tile.title)
  }
}

/// The tiles shown in the top sites UI. Owns the favorites fetch, the most visited observation and
/// the mode rules so that consumers only render `tiles`.
@Observable
class TopSitesTileSource: NSObject {
  /// Caps favorites and most visited to the same number of tiles.
  static let maxNumberOfTiles: Int = 20

  /// The tiles to display, in display order.
  private(set) var tiles: [TopSiteTile] = []

  /// Reordering only applies to favorites, and only when there is more than one to reorder.
  var isReorderingEnabled: Bool {
    mode == .favourite && tiles.count > 1
  }

  private let isPrivateBrowsing: Bool
  private let frc: NSFetchedResultsController<Favorite>
  private let mostVisitedSites: MostVisitedSites?
  private var mostVisitedObservation: MostVisitedSitesScopedObservation?
  private var mostVisitedTiles: [NTPTile] = []

  init(mostVisitedSites: MostVisitedSites?, isPrivateBrowsing: Bool) {
    self.isPrivateBrowsing = isPrivateBrowsing
    self.mostVisitedSites = mostVisitedSites
    self.frc = Favorite.frc()

    super.init()

    frc.fetchRequest.fetchLimit = Self.maxNumberOfTiles
    frc.delegate = self
    do {
      try frc.performFetch()
    } catch {
      Logger.module.error("Favorites fetch error")
    }

    Preferences.NewTabPage.topSitesMode.observe(from: self)
    updateMostVisitedObservation()
    updateTiles()
  }

  deinit {
    mostVisitedObservation?.invalidate()
  }

  // MARK: - Tiles

  /// The chosen mode after private browsing rules are applied: private browsing shows nothing
  /// unless the user has chosen favorites.
  private var mode: TopSitesMode {
    let mode = Preferences.NewTabPage.topSitesMode.value
    if isPrivateBrowsing {
      return mode == .favourite ? .favourite : TopSitesMode.none
    }
    return mode
  }

  /// Rebuilds `tiles` from whichever source the current mode draws from.
  private func updateTiles() {
    let updated: [TopSiteTile]
    switch mode {
    case TopSitesMode.none:
      updated = []
    case .favourite:
      updated = (frc.fetchedObjects ?? []).compactMap { TopSiteTile($0) }
    case .mostVisited:
      updated = mostVisitedTiles.map { TopSiteTile($0) }
    }
    if updated != tiles {
      tiles = updated
    }
  }

  private func updateMostVisitedObservation() {
    if mode == .mostVisited {
      guard mostVisitedObservation == nil else { return }
      mostVisitedObservation = mostVisitedSites?.addMostVisitedURLsObserver(
        self,
        maxNumSites: UInt(Self.maxNumberOfTiles)
      )
    } else {
      mostVisitedObservation?.invalidate()
      mostVisitedObservation = nil
      mostVisitedTiles = []
    }
  }
}

extension TopSitesTileSource: NSFetchedResultsControllerDelegate {
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    try? frc.performFetch()
    updateTiles()
  }
}

extension TopSitesTileSource: MostVisitedSitesObserver {
  func mostVisitedSitesDidUpdateTiles(_ tiles: [NTPTile]) {
    mostVisitedTiles = tiles
    updateTiles()
  }

  func mostVisitedSitesDidUpdateFavicon(for url: URL?) {
    // Favicons are loaded by the cells themselves.
  }
}

extension TopSitesTileSource: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    guard key == Preferences.NewTabPage.topSitesMode.key else { return }
    updateMostVisitedObservation()
    updateTiles()
  }
}
