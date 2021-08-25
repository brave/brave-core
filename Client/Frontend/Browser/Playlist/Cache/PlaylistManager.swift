// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import AVFoundation
import Shared
import CoreData
import Data
import BraveShared

private let log = Logger.browserLogger

protocol PlaylistManagerDelegate: AnyObject {
    func onDownloadProgressUpdate(id: String, percentComplete: Double)
    func onDownloadStateChanged(id: String, state: PlaylistDownloadManager.DownloadState, displayName: String?, error: Error?)
    
    func controllerDidChange(_ anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?)
    func controllerDidChangeContent()
    func controllerWillChangeContent()
}

class PlaylistManager: NSObject {
    static let shared = PlaylistManager()
    weak var delegate: PlaylistManagerDelegate?
    
    private let downloadManager = PlaylistDownloadManager()
    private let frc = PlaylistItem.frc()
    private var didRestoreSession = false
    
    private override init() {
        super.init()
        
        downloadManager.delegate = self
        frc.delegate = self
    }
    
    var numberOfAssets: Int {
        frc.fetchedObjects?.count ?? 0
    }
    
    func itemAtIndex(_ index: Int) -> PlaylistInfo {
        PlaylistInfo(item: frc.object(at: IndexPath(row: index, section: 0)))
    }
    
    func assetAtIndex(_ index: Int) -> AVURLAsset {
        let item = itemAtIndex(index)
        return asset(for: item.pageSrc, mediaSrc: item.src)
    }
    
    func index(of pageSrc: String) -> Int? {
        frc.fetchedObjects?.firstIndex(where: { $0.pageSrc == pageSrc })
    }
    
    func reorderItems(from sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath, completion: (() -> Void)?) {
        guard var objects = frc.fetchedObjects else {
            ensureMainThread {
                completion?()
            }
            return
        }

        frc.managedObjectContext.perform { [weak self] in
            defer {
                ensureMainThread {
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
                log.error(error)
            }
        }
    }
    
    func state(for pageSrc: String) -> PlaylistDownloadManager.DownloadState {
        if downloadManager.downloadTask(for: pageSrc) != nil {
            return .inProgress
        }
        
        if let assetUrl = downloadManager.localAsset(for: pageSrc)?.url {
            if FileManager.default.fileExists(atPath: assetUrl.path) {
                return .downloaded
            }
        }

        return .invalid
    }
    
    func sizeOfDownloadedItem(for pageSrc: String) -> String? {
        var isDirectory: ObjCBool = false
        if let asset = downloadManager.localAsset(for: pageSrc),
           FileManager.default.fileExists(atPath: asset.url.path, isDirectory: &isDirectory) {
            
            let formatter = ByteCountFormatter().then {
                $0.zeroPadsFractionDigits = true
                $0.countStyle = .file
            }
            
            if isDirectory.boolValue || asset.url.pathExtension.lowercased() == "movpkg" {
                let properties: [URLResourceKey] = [.isRegularFileKey, .totalFileAllocatedSizeKey]
                guard let enumerator = FileManager.default.enumerator(at: asset.url,
                                                                      includingPropertiesForKeys: properties,
                                                                      options: .skipsHiddenFiles,
                                                                      errorHandler: nil) else {
                    return nil
                }
                
                let sizes = enumerator.compactMap({ try? ($0 as? URL)?
                                    .resourceValues(forKeys: Set(properties)) })
                                    .filter({ $0.isRegularFile == true })
                                    .compactMap({ $0.totalFileAllocatedSize })
                                    .compactMap({ Int64($0) })
                
                return formatter.string(fromByteCount: Int64(sizes.reduce(0, +)))
            }
            
            if let size = try? FileManager.default.attributesOfItem(atPath: asset.url.path)[.size] as? Int {
                return formatter.string(fromByteCount: Int64(size))
            }
        }
        return nil
    }
    
    func reloadData() {
        do {
            try frc.performFetch()
        } catch {
            log.error(error)
        }
    }
    
    func restoreSession() {
        downloadManager.restoreSession() { [weak self] in
            self?.reloadData()
        }
    }
    
    func download(item: PlaylistInfo) {
        guard downloadManager.downloadTask(for: item.pageSrc) == nil, let assetUrl = URL(string: item.src) else { return }
        
        MediaResourceManager.getMimeType(assetUrl) { [weak self] mimeType in
            guard let self = self, let mimeType = mimeType?.lowercased() else { return }

            if mimeType.contains("x-mpegurl") || mimeType.contains("application/vnd.apple.mpegurl") || mimeType.contains("mpegurl") {
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
    
    func cancelDownload(item: PlaylistInfo) {
        downloadManager.cancelDownload(item: item)
    }
    
    @discardableResult
    func delete(item: PlaylistInfo) -> Bool {
        cancelDownload(item: item)
        
        if let cacheItem = PlaylistItem.getItem(pageSrc: item.pageSrc),
           cacheItem.cachedData != nil {
            // Do NOT delete the item if we can't delete it's local cache.
            // That will cause zombie items.
            if deleteCache(item: item) {
                PlaylistItem.removeItem(item)
                delegate?.onDownloadStateChanged(id: item.pageSrc, state: .invalid, displayName: nil, error: nil)
                return true
            }
            return false
        } else {
            PlaylistItem.removeItem(item)
            delegate?.onDownloadStateChanged(id: item.pageSrc, state: .invalid, displayName: nil, error: nil)
            return true
        }
    }
    
    @discardableResult
    func deleteCache(item: PlaylistInfo) -> Bool {
        cancelDownload(item: item)
        
        if let cacheItem = PlaylistItem.getItem(pageSrc: item.pageSrc),
           let cachedData = cacheItem.cachedData {
            var isStale = false
            
            do {
                let url = try URL(resolvingBookmarkData: cachedData, bookmarkDataIsStale: &isStale)
                if FileManager.default.fileExists(atPath: url.path) {
                    try FileManager.default.removeItem(atPath: url.path)
                    PlaylistItem.updateCache(pageSrc: item.pageSrc, cachedData: nil)
                    delegate?.onDownloadStateChanged(id: item.pageSrc, state: .invalid, displayName: nil, error: nil)
                }
                return true
            } catch {
                log.error("An error occured deleting Playlist Cached Item \(item.name): \(error)")
                return false
            }
        }
        return true
    }
    
    func deleteAllItems(cacheOnly: Bool) {
        // This is the only way to have the system kill picture in picture as the restoration controller is deallocated
        // And that means the video is deallocated, its AudioSession is stopped, and the Picture-In-Picture controller is deallocated.
        // This is because `AVPictureInPictureController` is NOT a view controller and there is no way to dismiss it
        // other than to deallocate the restoration controller.
        // We could also call `AVPictureInPictureController.stopPictureInPicture` BUT we'd still have to deallocate all resources.
        // At least this way, we deallocate both AND pip is stopped in the destructor of `PlaylistViewController->ListController`
        (UIApplication.shared.delegate as? AppDelegate)?.playlistRestorationController = nil
 
        guard let playlistItems = frc.fetchedObjects else {
            log.error("An error occured while fetching Playlist Objects")
            return
        }
        
        for playlistItem in playlistItems {
            let item = PlaylistInfo(item: playlistItem)
            
            if !deleteCache(item: item) {
                continue
            }
            
            if !cacheOnly {
                PlaylistItem.removeItem(item)
            }
        }
    }
    
    func autoDownload(item: PlaylistInfo) {
        guard let downloadType = PlayListDownloadType(rawValue: Preferences.Playlist.autoDownloadVideo.value) else {
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
    
    func isDiskSpaceEncumbered() -> Bool {
        let freeSpace = availableDiskSpace() ?? 0
        let totalSpace = totalDiskSpace() ?? 0
        let usedSpace = totalSpace - freeSpace
        
        // If disk space is 90% used
        return totalSpace == 0 || (Double(usedSpace) / Double(totalSpace)) * 100.0 >= 90.0
    }
    
    private func availableDiskSpace() -> Int64? {
        do {
            return try URL(fileURLWithPath: NSHomeDirectory() as String).resourceValues(forKeys: [.volumeAvailableCapacityForImportantUsageKey]).volumeAvailableCapacityForImportantUsage
        } catch {
            log.error("Error Retrieving Disk Space: \(error)")
        }
        return nil
    }
    
    private func totalDiskSpace() -> Int64? {
        do {
            if let result = try URL(fileURLWithPath: NSHomeDirectory() as String).resourceValues(forKeys: [.volumeTotalCapacityKey]).volumeTotalCapacity {
                return Int64(result)
            }
        } catch {
            log.error("Error Retrieving Disk Space: \(error)")
        }
        return nil
    }
}

extension PlaylistManager {
    private func asset(for pageSrc: String, mediaSrc: String) -> AVURLAsset {
        if let task = downloadManager.downloadTask(for: pageSrc) {
            return task.asset
        }
        
        if let asset = downloadManager.localAsset(for: pageSrc) {
            return asset
        }
        
        return AVURLAsset(url: URL(string: mediaSrc)!)
    }
}

extension PlaylistManager: PlaylistDownloadManagerDelegate {
    func onDownloadProgressUpdate(id: String, percentComplete: Double) {
        delegate?.onDownloadProgressUpdate(id: id, percentComplete: percentComplete)
    }
    
    func onDownloadStateChanged(id: String, state: PlaylistDownloadManager.DownloadState, displayName: String?, error: Error?) {
        delegate?.onDownloadStateChanged(id: id, state: state, displayName: displayName, error: error)
    }
}

extension PlaylistManager: NSFetchedResultsControllerDelegate {
    func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {
        
        delegate?.controllerDidChange(anObject, at: indexPath, for: type, newIndexPath: newIndexPath)
    }
    
    func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
        delegate?.controllerDidChangeContent()
    }
    
    func controllerWillChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
        delegate?.controllerWillChangeContent()
    }
}

extension AVAsset {
    func displayNames(for mediaSelection: AVMediaSelection) -> String? {
        var names = ""
        for mediaCharacteristic in availableMediaCharacteristicsWithMediaSelectionOptions {
            guard let mediaSelectionGroup = mediaSelectionGroup(forMediaCharacteristic: mediaCharacteristic),
                  let option = mediaSelection.selectedMediaOption(in: mediaSelectionGroup) else { continue }

            if names.isEmpty {
                names += " " + option.displayName
            } else {
                names += ", " + option.displayName
            }
        }

        return names.isEmpty ? nil : names
    }
}
