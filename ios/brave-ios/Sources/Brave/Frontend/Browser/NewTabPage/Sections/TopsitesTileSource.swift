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

/// Identifies a tile across updates. Used by consumers backed by a diffable data source.
enum TopsiteID: Hashable {
  case favorite(NSManagedObjectID)
  case mostVisited(URL)
}

extension TopsiteViewModel {
  var id: TopsiteID {
    switch source {
    case .favorite(let favorite): return .favorite(favorite.objectID)
    case .mostVisited(let tile): return .mostVisited(tile.url as URL)
    }
  }
}

protocol TopsitesTileSourceObserver: AnyObject {
  /// The tiles or the mode changed and consumers should reload.
  func topsitesTileSourceDidChangeTiles(_ source: TopsitesTileSource)
  /// A most visited favicon became available. Consumers that load their own favicons may ignore
  /// this.
  func topsitesTileSource(_ source: TopsitesTileSource, didUpdateFaviconFor url: URL?)
}

extension TopsitesTileSourceObserver {
  func topsitesTileSource(_ source: TopsitesTileSource, didUpdateFaviconFor url: URL?) {}
}

/// The tiles shown in the topsites UI. Owns the favorites fetch, the most visited observation and
/// the mode rules so that consumers only render.
class TopsitesTileSource: NSObject {
  /// Caps favorites and most visited to the same number of tiles.
  static let maxNumberOfTiles: Int = 20

  private let isPrivateBrowsing: Bool
  private let frc: NSFetchedResultsController<Favorite>
  private let mostVisitedSites: MostVisitedSites?
  private var mostVisitedObservation: MostVisitedSitesScopedObservation?
  private var mostVisitedTiles: [NTPTile] = []
  private let observers = NSHashTable<AnyObject>.weakObjects()

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

    Preferences.NewTabPage.topsitesMode.observe(from: self)
    updateMostVisitedObservation()
  }

  deinit {
    mostVisitedObservation?.invalidate()
  }

  // MARK: - Mode

  /// The chosen mode after private browsing rules are applied: private browsing shows nothing
  /// unless the user has chosen favorites.
  var effectiveMode: TopsitesMode {
    let mode = Preferences.NewTabPage.topsitesMode.value
    if isPrivateBrowsing {
      return mode == .favourite ? .favourite : TopsitesMode.none
    }
    return mode
  }

  // MARK: - Tiles

  var count: Int {
    switch effectiveMode {
    case TopsitesMode.none:
      return 0
    case .favourite:
      return frc.fetchedObjects?.count ?? 0
    case .mostVisited:
      return mostVisitedTiles.count
    }
  }

  var isEmpty: Bool {
    count == 0
  }

  subscript(index: Int) -> TopsiteViewModel? {
    switch effectiveMode {
    case TopsitesMode.none:
      return nil
    case .favourite:
      guard let favorite = frc.fetchedObjects?[safe: index] else { return nil }
      return TopsiteViewModel(source: .favorite(favorite))
    case .mostVisited:
      guard let tile = mostVisitedTiles[safe: index] else { return nil }
      return TopsiteViewModel(source: .mostVisited(tile))
    }
  }

  /// Tile identities in display order, for building diffable snapshots.
  var identifiers: [TopsiteID] {
    switch effectiveMode {
    case TopsitesMode.none:
      return []
    case .favourite:
      return (frc.fetchedObjects ?? []).map { .favorite($0.objectID) }
    case .mostVisited:
      return mostVisitedTiles.map { .mostVisited($0.url as URL) }
    }
  }

  /// Resolves an identity back to a tile, or nil once the tile is gone.
  func tile(for identifier: TopsiteID) -> TopsiteViewModel? {
    switch identifier {
    case .favorite(let objectID):
      guard effectiveMode == .favourite, let favorite = Favorite.get(with: objectID) else {
        return nil
      }
      return TopsiteViewModel(source: .favorite(favorite))
    case .mostVisited(let url):
      guard effectiveMode == .mostVisited,
        let tile = mostVisitedTiles.first(where: { $0.url as URL == url })
      else {
        return nil
      }
      return TopsiteViewModel(source: .mostVisited(tile))
    }
  }

  /// Reordering only applies to favorites, and only when there is more than one to reorder.
  var isReorderingEnabled: Bool {
    effectiveMode == .favourite && count > 1
  }

  // MARK: - Actions

  func exclude(_ tile: NTPTile) {
    mostVisitedSites?.setBlocked(true, for: tile.url)
  }

  func refresh() {
    mostVisitedSites?.refresh()
  }

  // MARK: - Observers

  func addObserver(_ observer: TopsitesTileSourceObserver) {
    observers.add(observer)
  }

  func removeObserver(_ observer: TopsitesTileSourceObserver) {
    observers.remove(observer)
  }

  private func notifyTilesChanged() {
    for case let observer as TopsitesTileSourceObserver in observers.allObjects {
      observer.topsitesTileSourceDidChangeTiles(self)
    }
  }

  private func updateMostVisitedObservation() {
    if effectiveMode == .mostVisited {
      guard mostVisitedObservation == nil else { return }
      // Tile types must be set first: the observer receives the current set of tiles on attach.
      mostVisitedSites?.enableTopSitesOnlyTileTypes()
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

extension TopsitesTileSource: NSFetchedResultsControllerDelegate {
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    try? frc.performFetch()
    // Core Data may notify mid-batch, where reloading synchronously is unsafe.
    DispatchQueue.main.async { [weak self] in
      self?.notifyTilesChanged()
    }
  }
}

extension TopsitesTileSource: MostVisitedSitesObserver {
  func mostVisitedSitesDidUpdateTiles(_ tiles: [NTPTile]) {
    mostVisitedTiles = tiles
    notifyTilesChanged()
  }

  func mostVisitedSitesDidUpdateFavicon(for url: URL?) {
    for case let observer as TopsitesTileSourceObserver in observers.allObjects {
      observer.topsitesTileSource(self, didUpdateFaviconFor: url)
    }
  }
}

extension TopsitesTileSource: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    guard key == Preferences.NewTabPage.topsitesMode.key else { return }
    updateMostVisitedObservation()
    notifyTilesChanged()
  }
}
