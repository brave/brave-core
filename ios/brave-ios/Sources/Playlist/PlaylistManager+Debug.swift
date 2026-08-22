// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreData
import Data
import Foundation

private enum PlaylistDebug {
  static var forceDiskSpaceEncumbered = false
}

/// QA tooling for playlist cache reclamation. Surfaced in Settings → Developer Options → Playlist Debug.
extension PlaylistManager {
  static var isSimulatingDiskSpaceEncumbered: Bool {
    PlaylistDebug.forceDiskSpaceEncumbered
  }

  @_spi(PlaylistDebug) public var debugForceDiskSpaceEncumbered: Bool {
    get { PlaylistDebug.forceDiskSpaceEncumbered }
    set { PlaylistDebug.forceDiskSpaceEncumbered = newValue }
  }

  @_spi(PlaylistDebug) public var debugIsCacheReclamationEnabled: Bool {
    isCacheReclamationEnabled
  }

  @_spi(PlaylistDebug) public func debugDiskUsedPercent() -> Double? {
    guard let freeSpace = Self.debugAvailableDiskSpace(),
      let totalSpace = Self.debugTotalDiskSpace(),
      totalSpace > 0
    else { return nil }
    let usedSpace = totalSpace - freeSpace
    return (Double(usedSpace) / Double(totalSpace)) * 100.0
  }

  @_spi(PlaylistDebug) public func debugCachedItemCount() -> Int {
    let request = NSFetchRequest<PlaylistItem>(entityName: "PlaylistItem")
    request.predicate = NSPredicate(format: "cachedData != nil")
    return (try? DataController.swiftUIContext.count(for: request)) ?? 0
  }

  @_spi(PlaylistDebug) @MainActor public var debugOutOfSpaceRetryItemCount: Int {
    itemsRetriedAfterOutOfSpace.count
  }

  @_spi(PlaylistDebug) @MainActor public func debugResetOutOfSpaceRetryTracking() {
    itemsRetriedAfterOutOfSpace.removeAll()
  }

  @_spi(PlaylistDebug) @MainActor public func debugRunCacheReclamation() async {
    await reclaimSpaceIfNeeded()
  }

  private static func debugAvailableDiskSpace() -> Int64? {
    try? URL(fileURLWithPath: NSHomeDirectory()).resourceValues(forKeys: [
      .volumeAvailableCapacityForImportantUsageKey
    ]).volumeAvailableCapacityForImportantUsage
  }

  private static func debugTotalDiskSpace() -> Int64? {
    guard
      let capacity = try? URL(fileURLWithPath: NSHomeDirectory()).resourceValues(forKeys: [
        .volumeTotalCapacityKey
      ]).volumeTotalCapacity
    else { return nil }
    return Int64(capacity)
  }
}
