// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveShared
import Combine
import CoreData
import Data
import Foundation
import Preferences
import Shared
import SwiftUI
import UIKit
import os.log

public class PlaylistManager: NSObject {
  public static let shared = PlaylistManager()

  private var assetInformation = [PlaylistAssetFetcher]()
  private let downloadManager = PlaylistDownloadManager()
  private var frc = PlaylistItem.frc()
  private var didRestoreSession = false

  private var _playbackTask: Task<Void, Error>?

  public var playbackTask: Task<Void, Error>? {
    get {
      _playbackTask
    }

    set(newValue) {
      _playbackTask?.cancel()
      _playbackTask = newValue
    }
  }

  // Observers
  private let onContentWillChange = PassthroughSubject<Void, Never>()
  private let onContentDidChange = PassthroughSubject<Void, Never>()
  private let onObjectChange = PassthroughSubject<
    (
      object: Any,
      indexPath: IndexPath?,
      type: NSFetchedResultsChangeType,
      newIndexPath: IndexPath?
    ), Never
  >()

  private let onDownloadProgressUpdate = PassthroughSubject<
    (
      id: String,
      percentComplete: Double
    ), Never
  >()
  private let onDownloadStateChanged = PassthroughSubject<
    (
      id: String,
      state: PlaylistDownloadManager.DownloadState,
      displayName: String?,
      error: Error?
    ), Never
  >()
  private let onCurrentFolderChanged = PassthroughSubject<(), Never>()
  private let onFolderDeleted = PassthroughSubject<(), Never>()

  private override init() {
    super.init()

    downloadManager.delegate = self
    frc.delegate = self

    let sevenDays =
      Preferences.Playlist.lastCacheDataCleanupDate.value?.addingTimeInterval(7.days) ?? Date.now
    if Date.now >= sevenDays {
      Preferences.Playlist.lastCacheDataCleanupDate.value = Date.now

      Task {
        // Delete system cache always on startup.
        await deleteUserManagedAssets()

        // Delete dangling cache always on startup.
        await deleteDanglingManagedAssets()
      }
    }
  }

  public var currentFolder: PlaylistFolder? {
    didSet {
      frc.delegate = nil

      if let currentFolder = currentFolder {
        // Only return an FRC for the specified folder
        frc = PlaylistItem.frc(parentFolder: currentFolder)
      } else {
        // Return every folder, including the "Saved" folder
        frc = PlaylistItem.allFoldersFRC()
      }

      frc.delegate = self
      reloadData()

      onCurrentFolderChanged.send()
    }
  }

  public var onFolderRemovedOrUpdated: AnyPublisher<Void, Never> {
    onFolderDeleted.eraseToAnyPublisher()
  }

  public var contentWillChange: AnyPublisher<Void, Never> {
    onContentWillChange.eraseToAnyPublisher()
  }

  public var contentDidChange: AnyPublisher<Void, Never> {
    onContentDidChange.eraseToAnyPublisher()
  }

  public var objectDidChange:
    AnyPublisher<
      (
        object: Any, indexPath: IndexPath?, type: NSFetchedResultsChangeType,
        newIndexPath: IndexPath?
      ), Never
    >
  {
    onObjectChange.eraseToAnyPublisher()
  }

  public var downloadProgressUpdated: AnyPublisher<(id: String, percentComplete: Double), Never> {
    onDownloadProgressUpdate.eraseToAnyPublisher()
  }

  public var downloadStateChanged:
    AnyPublisher<
      (
        id: String, state: PlaylistDownloadManager.DownloadState, displayName: String?,
        error: Error?
      ), Never
    >
  {
    onDownloadStateChanged.eraseToAnyPublisher()
  }

  public var onCurrentFolderDidChange: AnyPublisher<(), Never> {
    onCurrentFolderChanged.eraseToAnyPublisher()
  }

  public var allItems: [PlaylistInfo] {
    frc.fetchedObjects?.map({ PlaylistInfo(item: $0) }) ?? []
  }

  public var numberOfAssets: Int {
    frc.fetchedObjects?.count ?? 0
  }

  public var fetchedObjects: [PlaylistItem] {
    frc.fetchedObjects ?? []
  }

  public func updateLastPlayed(item: PlaylistInfo, playTime: Double) {
    let lastPlayedTime = Preferences.Playlist.playbackLeftOff.value ? playTime : 0.0
    Preferences.Playlist.lastPlayedItemUrl.value = item.pageSrc
    PlaylistItem.updateLastPlayed(
      itemId: item.tagId,
      pageSrc: item.pageSrc,
      lastPlayedOffset: lastPlayedTime
    )
  }

  public func itemAtIndex(_ index: Int) -> PlaylistInfo? {
    if index >= 0 && index < numberOfAssets {
      return PlaylistInfo(item: frc.object(at: IndexPath(row: index, section: 0)))
    }
    return nil
  }

  @available(*, deprecated, renamed: "assetAtIndex(_:)", message: "Use async version")
  public func assetAtIndexSynchronous(_ index: Int) -> AVURLAsset? {
    if let item = itemAtIndex(index) {
      return assetSynchronous(for: item.tagId, mediaSrc: item.src)
    }
    return nil
  }

  public func assetAtIndex(_ index: Int) async -> AVURLAsset? {
    let data: (tagId: String, src: String)? = await MainActor.run {
      if let item = itemAtIndex(index) {
        return (item.tagId, item.src)
      }
      return nil
    }
    if let data = data {
      return await asset(for: data.tagId, mediaSrc: data.src)
    }
    return nil
  }

  public func index(of itemId: String) -> Int? {
    frc.fetchedObjects?.firstIndex(where: { $0.uuid == itemId })
  }

  public func reorderItems(
    from sourceIndexPath: IndexPath,
    to destinationIndexPath: IndexPath,
    completion: (() -> Void)?
  ) {
    guard var objects = frc.fetchedObjects else {
      DispatchQueue.main.async {
        completion?()
      }
      return
    }

    frc.managedObjectContext.perform { [weak self] in
      defer {
        DispatchQueue.main.async {
          completion?()
        }
      }

      guard let self = self else { return }

      let src = self.frc.object(at: sourceIndexPath)
      objects.remove(at: sourceIndexPath.row)
      objects.insert(src, at: destinationIndexPath.row)

      for (order, item) in objects.enumerated().reversed() {
        item.order = Int32(order)
      }

      do {
        try self.frc.managedObjectContext.save()
      } catch {
        Logger.module.error("\(error.localizedDescription)")
      }
    }
  }

  @available(*, deprecated, renamed: "downloadState(for:)", message: "Use async version")
  public func state(for itemId: String) -> PlaylistDownloadManager.DownloadState {
    if downloadManager.downloadTask(for: itemId) != nil {
      return .inProgress
    }

    if let assetUrl = downloadManager.localAssetSynchronous(for: itemId)?.url {
      if FileManager.default.fileExists(atPath: assetUrl.path) {
        return .downloaded
      }
    }

    return .invalid
  }

  public func downloadState(for itemId: String) async -> PlaylistDownloadManager.DownloadState {
    if downloadManager.downloadTask(for: itemId) != nil {
      return .inProgress
    }

    if let assetUrl = await downloadManager.localAsset(for: itemId)?.url {
      if await AsyncFileManager.default.fileExists(atPath: assetUrl.path) {
        return .downloaded
      }
    }

    return .invalid
  }

  @available(iOS, deprecated)
  public func sizeOfDownloadedItemSynchronous(for itemId: String) -> String? {
    var isDirectory: ObjCBool = false
    if let asset = downloadManager.localAssetSynchronous(for: itemId),
      FileManager.default.fileExists(atPath: asset.url.path, isDirectory: &isDirectory)
    {

      let formatter = ByteCountFormatter().then {
        $0.zeroPadsFractionDigits = true
        $0.countStyle = .file
      }

      if isDirectory.boolValue || asset.url.pathExtension.lowercased() == "movpkg" {
        let properties: [URLResourceKey] = [.isRegularFileKey, .totalFileAllocatedSizeKey]
        guard
          let enumerator = FileManager.default.enumerator(
            at: asset.url,
            includingPropertiesForKeys: properties,
            options: .skipsHiddenFiles,
            errorHandler: nil
          )
        else {
          return nil
        }

        let sizes = enumerator.compactMap({
          try? ($0 as? URL)?
            .resourceValues(forKeys: Set(properties))
        })
        .filter({ $0.isRegularFile == true })
        .compactMap({ $0.totalFileAllocatedSize })
        .compactMap({ Int64($0) })

        return formatter.string(fromByteCount: Int64(sizes.reduce(0, +)))
      }

      if let size = try? FileManager.default.attributesOfItem(atPath: asset.url.path)[.size] as? Int
      {
        return formatter.string(fromByteCount: Int64(size))
      }
    }
    return nil
  }

  public func reloadData() {
    do {
      try frc.performFetch()
    } catch {
      Logger.module.error("\(error.localizedDescription)")
    }
  }

  public func restoreSession() {
    if !didRestoreSession {
      downloadManager.restoreSession { [weak self] in
        self?.reloadData()
      }
    }
  }

  public func setupPlaylistFolder() {
    if let savedFolder = PlaylistFolder.getFolder(uuid: PlaylistFolder.savedFolderUUID) {
      if savedFolder.title != Strings.Playlist.defaultPlaylistTitle {
        // This title may change so we should update it
        savedFolder.title = Strings.Playlist.defaultPlaylistTitle
      }
    } else {
      PlaylistFolder.addFolder(
        title: Strings.Playlist.defaultPlaylistTitle,
        uuid: PlaylistFolder.savedFolderUUID
      ) { uuid in
        Logger.module.debug("Created Playlist Folder: \(uuid)")
      }
    }
  }

  public func download(item: PlaylistInfo) {
    guard downloadManager.downloadTask(for: item.tagId) == nil, let assetUrl = URL(string: item.src)
    else { return }
    Task {
      if assetUrl.scheme == "data" {
        DispatchQueue.main.async {
          self.downloadManager.downloadDataAsset(assetUrl, for: item)
        }
        return
      }

      let mimeType = await PlaylistMediaStreamer.getMimeType(assetUrl)
      guard let mimeType = mimeType?.lowercased() else { return }

      if mimeType.contains("x-mpegurl") || mimeType.contains("application/vnd.apple.mpegurl")
        || mimeType.contains("mpegurl")
      {
        DispatchQueue.main.async {
          self.downloadManager.downloadHLSAsset(assetUrl, for: item)
        }
      } else {
        DispatchQueue.main.async {
          self.downloadManager.downloadFileAsset(assetUrl, for: item)
        }
      }
    }
  }

  public func cancelDownload(itemId: String) {
    downloadManager.cancelDownload(itemId: itemId)
  }

  @MainActor public func delete(folder: PlaylistFolder) async -> Bool {
    var success = true
    var itemsToDelete = [PlaylistInfo]()

    for playlistItem in folder.playlistItems ?? [] {
      let item = PlaylistInfo(item: playlistItem)
      cancelDownload(itemId: item.tagId)

      if let index = assetInformation.firstIndex(where: { $0.itemId == item.tagId }) {
        let assetFetcher = self.assetInformation.remove(at: index)
        assetFetcher.cancelLoading()
      }

      if !(await deleteCache(item: item)) {
        // If we cannot delete an item's cache for any given reason,
        // Do NOT delete the folder containing the item.
        // Delete all other items.
        success = false
      } else {
        itemsToDelete.append(item)
      }
    }

    if success, currentFolder?.objectID == folder.objectID {
      currentFolder = nil
    }

    // Delete items from the folder
    return await withCheckedContinuation { continuation in
      PlaylistItem.removeItems(itemsToDelete) {
        // Attempt to delete the folder if we can
        if success, folder.uuid != PlaylistFolder.savedFolderUUID {
          PlaylistFolder.removeFolder(folder.uuid ?? "") { [weak self] in
            defer {
              continuation.resume(returning: success)
            }
            guard let self = self else {
              return
            }

            if self.currentFolder?.isDeleted == true {
              self.currentFolder = nil
            }

            self.onFolderDeleted.send()
            self.reloadData()
          }
        } else {
          if self.currentFolder?.isDeleted == true {
            self.currentFolder = nil
          }

          self.onFolderDeleted.send()
          self.reloadData()
          continuation.resume(returning: success)
        }
      }
    }
  }

  @discardableResult
  @MainActor public func delete(item: PlaylistInfo) async -> Bool {
    cancelDownload(itemId: item.tagId)

    if let index = assetInformation.firstIndex(where: { $0.itemId == item.tagId }) {
      let assetFetcher = self.assetInformation.remove(at: index)
      assetFetcher.cancelLoading()
    }

    if let cacheItem = PlaylistItem.getItem(uuid: item.tagId),
      cacheItem.cachedData != nil
    {
      // Do NOT delete the item if we can't delete it's local cache.
      // That will cause zombie items.
      if await deleteCache(item: item) {
        await withCheckedContinuation { continuation in
          PlaylistItem.removeItems([item]) {
            continuation.resume()
          }
        }
        onDownloadStateChanged(id: item.tagId, state: .invalid, displayName: nil, error: nil)
        return true
      }
      return false
    } else {
      await withCheckedContinuation { continuation in
        PlaylistItem.removeItems([item]) {
          continuation.resume()
        }
      }
      onDownloadStateChanged(id: item.tagId, state: .invalid, displayName: nil, error: nil)
      return true
    }
  }

  @discardableResult
  @MainActor public func deleteCache(item: PlaylistInfo) async -> Bool {
    cancelDownload(itemId: item.tagId)

    if let cacheItem = PlaylistItem.getItem(uuid: item.tagId),
      let cachedData = cacheItem.cachedData,
      !cachedData.isEmpty
    {
      var isStale = false

      do {
        let url = try URL(resolvingBookmarkData: cachedData, bookmarkDataIsStale: &isStale)
        if await AsyncFileManager.default.fileExists(atPath: url.path) {
          try await AsyncFileManager.default.removeItem(atPath: url.path)
          PlaylistItem.updateCache(uuid: item.tagId, pageSrc: item.pageSrc, cachedData: nil)
          onDownloadStateChanged(id: item.tagId, state: .invalid, displayName: nil, error: nil)
        }
        return true
      } catch {
        if (error as NSError).code == NSFileNoSuchFileError {
          PlaylistItem.updateCache(uuid: item.tagId, pageSrc: item.pageSrc, cachedData: nil)
          onDownloadStateChanged(id: item.tagId, state: .invalid, displayName: nil, error: nil)
          // Cached file is missing, nothing to delete
          return true
        }
        Logger.module.error(
          "An error occured deleting Playlist Cached Item \(cacheItem.name ?? item.tagId): \(error.localizedDescription)"
        )
        return false
      }
    }
    return true
  }

  @MainActor public func deleteAllItems(cacheOnly: Bool) async {
    guard let playlistItems = frc.fetchedObjects else {
      Logger.module.error("An error occured while fetching Playlist Objects")
      return
    }

    for item in playlistItems {
      let item = PlaylistInfo(item: item)
      if !(await deleteCache(item: item)) {
        continue
      }

      if !cacheOnly {
        PlaylistItem.removeItems([item])
      }
    }

    if !cacheOnly {
      assetInformation.forEach({ $0.cancelLoading() })
      assetInformation.removeAll()
    }

    // Delete playlist directory.
    // Though it should already be empty
    if let playlistDirectory = await PlaylistDownloadManager.playlistDirectory {
      do {
        try await AsyncFileManager.default.removeItem(at: playlistDirectory)
      } catch {
        Logger.module.error("Failed to delete Playlist Directory: \(error.localizedDescription)")
      }
    }

    let sevenDays =
      Preferences.Playlist.lastCacheDataCleanupDate.value?.addingTimeInterval(7.days) ?? Date.now
    if Date.now >= sevenDays {
      Preferences.Playlist.lastCacheDataCleanupDate.value = Date.now

      // Delete system cache always on startup.
      await deleteUserManagedAssets()

      // Delete dangling cache always on startup.
      await deleteDanglingManagedAssets()
    }
  }

  @MainActor private func deleteUserManagedAssets() async {
    // Cleanup System Cache Folder com.apple.UserManagedAssets*
    if let libraryPath = FileManager.default.urls(for: .libraryDirectory, in: .userDomainMask).first
    {
      do {
        let urls = try await AsyncFileManager.default.contentsOfDirectory(
          at: libraryPath,
          includingPropertiesForKeys: nil,
          options: [.skipsHiddenFiles]
        )
        for url in urls where url.absoluteString.contains("com.apple.UserManagedAssets") {
          do {
            let assets = try await AsyncFileManager.default.contentsOfDirectory(
              at: url,
              includingPropertiesForKeys: nil,
              options: [.skipsHiddenFiles]
            )
            assets.forEach({
              if let item = PlaylistItem.cachedItem(cacheURL: $0), let itemId = item.uuid {
                self.cancelDownload(itemId: itemId)
                PlaylistItem.updateCache(uuid: itemId, pageSrc: item.pageSrc, cachedData: nil)
              }
            })
          } catch {
            Logger.module.error(
              "Failed to update Playlist item cached state: \(error.localizedDescription)"
            )
          }

          do {
            try await AsyncFileManager.default.removeItem(at: url)
          } catch {
            Logger.module.error(
              "Deleting Playlist Item for \(url.absoluteString) failed: \(error.localizedDescription)"
            )
          }
        }
      } catch {
        Logger.module.error(
          "Deleting Playlist Incomplete Items failed: \(error.localizedDescription)"
        )
      }
    }
  }

  @MainActor private func deleteDanglingManagedAssets() async {
    if let playlistFolderPath = await PlaylistDownloadManager.playlistDirectory {
      let items = PlaylistItem.all().compactMap(\.cachedData)
      Task.detached {
        let cachedURLs = items.compactMap { cachedData in
          var isStale: Bool = false
          if let url = try? URL(resolvingBookmarkData: cachedData, bookmarkDataIsStale: &isStale) {
            return url
          }
          return nil
        }
        do {
          let urls = try FileManager.default.contentsOfDirectory(
            at: playlistFolderPath,
            includingPropertiesForKeys: nil,
            options: [.skipsHiddenFiles]
          )
          for url in urls {
            do {
              // Playlist doesn't contain such an offline item, so it's dangling somehow and should be deleted.
              if !cachedURLs.contains(where: { $0.path == url.path }) {
                try FileManager.default.removeItem(at: url)
              }
            } catch {
              Logger.module.error(
                "Deleting Dangling Playlist Item for \(url.absoluteString) failed: \(error.localizedDescription)"
              )
            }
          }
        } catch {
          Logger.module.error(
            "Deleting Dangling Playlist Incomplete Items failed: \(error.localizedDescription)"
          )
        }
      }
    }
  }

  public func autoDownload(item: PlaylistInfo) {
    guard
      let downloadType = PlayListDownloadType(
        rawValue: Preferences.Playlist.autoDownloadVideo.value
      )
    else {
      return
    }

    switch downloadType {
    case .on:
      PlaylistManager.shared.download(item: item)
    case .wifi:
      if DeviceInfo.hasWifiConnection() {
        PlaylistManager.shared.download(item: item)
      }
    case .off:
      break
    }
  }

  public func isDiskSpaceEncumbered() -> Bool {
    let freeSpace = availableDiskSpace() ?? 0
    let totalSpace = totalDiskSpace() ?? 0
    let usedSpace = totalSpace - freeSpace

    // If disk space is 90% used
    return totalSpace == 0 || (Double(usedSpace) / Double(totalSpace)) * 100.0 >= 90.0
  }

  private func availableDiskSpace() -> Int64? {
    do {
      return try URL(fileURLWithPath: NSHomeDirectory() as String).resourceValues(forKeys: [
        .volumeAvailableCapacityForImportantUsageKey
      ]).volumeAvailableCapacityForImportantUsage
    } catch {
      Logger.module.error("Error Retrieving Disk Space: \(error.localizedDescription)")
    }
    return nil
  }

  private func totalDiskSpace() -> Int64? {
    do {
      if let result = try URL(fileURLWithPath: NSHomeDirectory() as String).resourceValues(
        forKeys: [.volumeTotalCapacityKey]).volumeTotalCapacity
      {
        return Int64(result)
      }
    } catch {
      Logger.module.error("Error Retrieving Disk Space: \(error.localizedDescription)")
    }
    return nil
  }
}

extension PlaylistManager {
  @available(*, deprecated, renamed: "asset(for:mediaSrc:)", message: "Use async version")
  private func assetSynchronous(for itemId: String, mediaSrc: String) -> AVURLAsset {
    if let task = downloadManager.downloadTask(for: itemId) {
      return task.asset
    }

    if let asset = downloadManager.localAssetSynchronous(for: itemId) {
      return asset
    }

    return AVURLAsset(url: URL(string: mediaSrc)!, options: AVAsset.defaultOptions)
  }

  private func asset(for itemId: String, mediaSrc: String) async -> AVURLAsset {
    if let task = downloadManager.downloadTask(for: itemId) {
      return task.asset
    }

    if let asset = await downloadManager.localAsset(for: itemId) {
      return asset
    }

    return AVURLAsset(url: URL(string: mediaSrc)!, options: AVAsset.defaultOptions)
  }
}

extension PlaylistManager: PlaylistDownloadManagerDelegate {
  func onDownloadProgressUpdate(id: String, percentComplete: Double) {
    onDownloadProgressUpdate.send((id: id, percentComplete: percentComplete))
  }

  func onDownloadStateChanged(
    id: String,
    state: PlaylistDownloadManager.DownloadState,
    displayName: String?,
    error: Error?
  ) {
    onDownloadStateChanged.send((id: id, state: state, displayName: displayName, error: error))
  }
}

extension PlaylistManager: NSFetchedResultsControllerDelegate {
  public func controller(
    _ controller: NSFetchedResultsController<NSFetchRequestResult>,
    didChange anObject: Any,
    at indexPath: IndexPath?,
    for type: NSFetchedResultsChangeType,
    newIndexPath: IndexPath?
  ) {

    onObjectChange.send(
      (object: anObject, indexPath: indexPath, type: type, newIndexPath: newIndexPath)
    )
  }

  public func controllerDidChangeContent(
    _ controller: NSFetchedResultsController<NSFetchRequestResult>
  ) {
    onContentDidChange.send(())
  }

  public func controllerWillChangeContent(
    _ controller: NSFetchedResultsController<NSFetchRequestResult>
  ) {
    onContentWillChange.send(())
  }
}

extension PlaylistManager {
  public func getAssetDuration(item: PlaylistInfo, _ completion: @escaping (TimeInterval?) -> Void)
  {
    if assetInformation.contains(where: { $0.itemId == item.tagId }) || item.src.isEmpty {
      completion(nil)
      return
    }

    fetchAssetDuration(item: item) { [weak self] duration in
      guard let self = self else {
        completion(nil)
        return
      }

      if let index = self.assetInformation.firstIndex(where: { $0.itemId == item.tagId }) {
        let assetFetcher = self.assetInformation.remove(at: index)
        assetFetcher.cancelLoading()
      }

      completion(duration)
    }
  }

  private func fetchAssetDuration(
    item: PlaylistInfo,
    _ completion: @escaping (TimeInterval?) -> Void
  ) {
    Task { @MainActor in
      completion(await fetchAssetDuration(item: item))
    }
  }

  @MainActor private func fetchAssetDuration(item: PlaylistInfo) async -> TimeInterval? {
    let tolerance: Double = 0.00001
    let distance = abs(item.duration.distance(to: 0.0))

    // If the database duration is live/indefinite
    if item.duration.isInfinite
      || abs(item.duration.distance(to: TimeInterval.greatestFiniteMagnitude)) < tolerance
    {
      return TimeInterval.infinity
    }

    // If the database duration is 0.0
    if distance >= tolerance {
      // Return the database duration
      return item.duration
    }

    // Attempt to retrieve the duration from the Asset file
    let asset: AVURLAsset
    if item.src.isEmpty || item.pageSrc.isEmpty {
      if let index = index(of: item.tagId), let urlAsset = await assetAtIndex(index) {
        asset = urlAsset
      } else {
        // Return the database duration
        return item.duration
      }
    } else {
      asset = await self.asset(for: item.tagId, mediaSrc: item.src)
    }

    // Accessing tracks blocks the main-thread if not already loaded
    // So we first need to check the track status before attempting to access it!
    var error: NSError?
    let trackStatus = asset.statusOfValue(forKey: "tracks", error: &error)

    if trackStatus == .loaded {
      if !asset.tracks.isEmpty,
        let track = asset.tracks(withMediaType: .video).first
          ?? asset.tracks(withMediaType: .audio).first
      {
        if track.timeRange.duration.isIndefinite {
          return TimeInterval.infinity
        } else {
          return track.timeRange.duration.seconds
        }
      }
    }

    // Accessing duration or commonMetadata blocks the main-thread if not already loaded
    // So we first need to check the track status before attempting to access it!
    let durationStatus = asset.statusOfValue(forKey: "duration", error: &error)
    if durationStatus == .loaded {
      // If it's live/indefinite
      if asset.duration.isIndefinite {
        return TimeInterval.infinity
      }

      // If it's a valid duration
      if abs(asset.duration.seconds.distance(to: 0.0)) >= tolerance {
        return asset.duration.seconds
      }
    }

    switch Reach().connectionStatus() {
    case .offline, .unknown:
      return item.duration  // Return the database duration
    case .online:
      break
    }

    assetInformation.append(PlaylistAssetFetcher(itemId: item.tagId, asset: asset))
    // We can't get the duration synchronously so we need to let the AVAsset load the media item
    // and hopefully we get a valid duration from that.
    return await Task.detached {
      do {
        let (_, loadedTracks, loadedDuration) = try await asset.load(
          .isPlayable,
          .tracks,
          .duration
        )
        var duration: CMTime = .zero
        if case .loaded = asset.status(of: .tracks) {
          if let track = loadedTracks.first(where: { $0.mediaType == .video })
            ?? loadedTracks.first(where: { $0.mediaType == .audio })
          {
            duration = track.timeRange.duration
          } else {
            duration = loadedDuration
          }
        } else {
          duration = loadedDuration
        }

        // Jump back to main for CoreData
        return await Task { @MainActor in
          if duration.isIndefinite {
            return TimeInterval.infinity
          } else if abs(duration.seconds.distance(to: 0.0)) > tolerance {
            let newItem = PlaylistInfo(
              name: item.name,
              src: item.src,
              pageSrc: item.pageSrc,
              pageTitle: item.pageTitle,
              mimeType: item.mimeType,
              duration: duration.seconds,
              lastPlayedOffset: 0.0,
              detected: item.detected,
              dateAdded: item.dateAdded,
              tagId: item.tagId,
              order: item.order,
              isInvisible: item.isInvisible
            )

            if PlaylistItem.itemExists(uuid: item.tagId)
              || PlaylistItem.itemExists(pageSrc: item.pageSrc)
            {
              await withCheckedContinuation { continuation in
                PlaylistItem.updateItem(newItem) {
                  continuation.resume()
                }
              }
              return duration.seconds
            } else {
              return duration.seconds
            }
          } else {
            return duration.seconds
          }
        }.value
      } catch {
        if (error as NSError).code == NSURLErrorNoPermissionsToReadFile {
          // Media item is expired.. permission is denied
          await MainActor.run {
            // Have to jump to main due to access of CoreData
            Logger.module.debug("Playlist Media Item Expired: \(item.pageSrc)")
          }
        } else {
          Logger.module.error("Failed to load asset details: \(error.localizedDescription)")
        }
        return nil
      }
    }.value
  }
}

extension PlaylistManager {
  @MainActor
  public static func syncSharedFolder(sharedFolderUrl: String) async throws {
    guard let folder = PlaylistFolder.getSharedFolder(sharedFolderUrl: sharedFolderUrl),
      let folderId = folder.uuid
    else {
      return
    }

    let model = try await PlaylistSharedFolderNetwork.fetchPlaylist(folderUrl: sharedFolderUrl)
    var oldItems = Set(folder.playlistItems?.map({ PlaylistInfo(item: $0) }) ?? [])
    let deletedItems = oldItems.subtracting(model.mediaItems)
    let newItems = Set(model.mediaItems).subtracting(oldItems)
    oldItems = []

    for deletedItem in deletedItems {
      await PlaylistManager.shared.delete(item: deletedItem)
    }

    if !newItems.isEmpty {
      await withCheckedContinuation { continuation in
        PlaylistItem.updateItems(Array(newItems), folderUUID: folderId, newETag: model.eTag) {
          continuation.resume()
        }
      }
    }
  }

  @MainActor
  public static func syncSharedFolders() async throws {
    let folderURLs = PlaylistFolder.getSharedFolders().compactMap({ $0.sharedFolderUrl })
    await withTaskGroup(of: Void.self) { group in
      folderURLs.forEach { url in
        group.addTask {
          try? await syncSharedFolder(sharedFolderUrl: url)
        }
      }
    }
  }
}

extension AVAsset {
  func displayNames(for mediaSelection: AVMediaSelection) -> String? {
    var names = ""
    for mediaCharacteristic in availableMediaCharacteristicsWithMediaSelectionOptions {
      guard
        let mediaSelectionGroup = mediaSelectionGroup(forMediaCharacteristic: mediaCharacteristic),
        let option = mediaSelection.selectedMediaOption(in: mediaSelectionGroup)
      else { continue }

      if names.isEmpty {
        names += " " + option.displayName
      } else {
        names += ", " + option.displayName
      }
    }

    return names.isEmpty ? nil : names
  }
}
