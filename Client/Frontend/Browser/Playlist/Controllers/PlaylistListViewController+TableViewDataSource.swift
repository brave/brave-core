// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import AVKit
import AVFoundation
import Data
import Shared

private let log = Logger.browserLogger

// MARK: UITableViewDataSource

extension PlaylistListViewController: UITableViewDataSource {
    private static let formatter = DateComponentsFormatter().then {
        $0.allowedUnits = [.day, .hour, .minute, .second]
        $0.unitsStyle = .abbreviated
        $0.maximumUnitCount = 2
    }
    
    private func getRelativeDateFormat(date: Date) -> String {
        let formatter = RelativeDateTimeFormatter()
        formatter.unitsStyle = .abbreviated
        formatter.dateTimeStyle = .numeric
        return formatter.localizedString(fromTimeInterval: date.timeIntervalSinceNow)
    }
    
    private func getAssetDuration(item: PlaylistInfo, _ completion: @escaping (TimeInterval?, AVAsset?) -> Void) -> PlaylistAssetFetcher? {
        let tolerance: Double = 0.00001
        let distance = abs(item.duration.distance(to: 0.0))
        
        // If the database duration is live/indefinite
        if item.duration.isInfinite ||
            abs(item.duration.distance(to: TimeInterval.greatestFiniteMagnitude)) < tolerance {
            completion(TimeInterval.infinity, nil)
            return nil
        }
        
        // If the database duration is 0.0
        if distance >= tolerance {
            // Return the database duration
            completion(item.duration, nil)
            return nil
        }
        
        guard let index = PlaylistManager.shared.index(of: item.pageSrc) else {
            completion(item.duration, nil) // Return the database duration
            return nil
        }
        
        // Attempt to retrieve the duration from the Asset file
        guard let asset = PlaylistManager.shared.assetAtIndex(index) else {
            completion(item.duration, nil) // Return the database duration
            return nil
        }
        
        // Accessing tracks blocks the main-thread if not already loaded
        // So we first need to check the track status before attempting to access it!
        var error: NSError?
        let trackStatus = asset.statusOfValue(forKey: "tracks", error: &error)
        if let error = error {
            log.error("AVAsset.statusOfValue error occurred: \(error)")
        }
        
        if trackStatus == .loaded {
            if !asset.tracks.isEmpty,
               let track = asset.tracks(withMediaType: .video).first ??
                            asset.tracks(withMediaType: .audio).first {
                if track.timeRange.duration.isIndefinite {
                    completion(TimeInterval.infinity, nil)
                } else {
                    completion(track.timeRange.duration.seconds, asset)
                }
                return nil
            }
        } else if trackStatus != .loading {
            log.debug("AVAsset.statusOfValue not loaded. Status: \(trackStatus)")
        }
        
        // Accessing duration or commonMetadata blocks the main-thread if not already loaded
        // So we first need to check the track status before attempting to access it!
        let durationStatus = asset.statusOfValue(forKey: "duration", error: &error)
        if let error = error {
            log.error("AVAsset.statusOfValue error occurred: \(error)")
        }
        
        if durationStatus == .loaded {
            // If it's live/indefinite
            if asset.duration.isIndefinite {
                completion(TimeInterval.infinity, asset)
                return nil
            }
            
            // If it's a valid duration
            if abs(asset.duration.seconds.distance(to: 0.0)) >= tolerance {
                completion(asset.duration.seconds, asset)
                return nil
            }
        } else if durationStatus != .loading {
            log.debug("AVAsset.statusOfValue not loaded. Status: \(durationStatus)")
        }
        
        switch Reach().connectionStatus() {
        case .offline, .unknown:
            completion(item.duration, nil) // Return the database duration
            return nil
        case .online:
            break
        }
        
        // We can't get the duration synchronously so we need to let the AVAsset load the media item
        // and hopefully we get a valid duration from that.
        asset.loadValuesAsynchronously(forKeys: ["playable", "tracks", "duration"]) {
            var error: NSError?
            let trackStatus = asset.statusOfValue(forKey: "tracks", error: &error)
            if let error = error {
                log.error("AVAsset.statusOfValue error occurred: \(error)")
            }
            
            let durationStatus = asset.statusOfValue(forKey: "tracks", error: &error)
            if let error = error {
                log.error("AVAsset.statusOfValue error occurred: \(error)")
            }
            
            if trackStatus == .cancelled || durationStatus == .cancelled {
                return
            }
            
            if trackStatus == .failed && durationStatus == .failed, let error = error {
                if error.code == NSURLErrorNoPermissionsToReadFile {
                    // Media item is expired.. permission is denied
                    log.debug("Playlist Media Item Expired: \(item.pageSrc)")
                    
                    ensureMainThread {
                        completion(nil, nil)
                    }
                } else {
                    log.error("An unknown error occurred while attempting to fetch track and duration information: \(error)")
                    
                    ensureMainThread {
                        completion(nil, nil)
                    }
                }
                
                return
            }
            
            var duration: CMTime = .zero
            if trackStatus == .loaded {
                if let track = asset.tracks(withMediaType: .video).first ?? asset.tracks(withMediaType: .audio).first {
                    duration = track.timeRange.duration
                } else {
                    duration = asset.duration
                }
            } else if durationStatus == .loaded {
                duration = asset.duration
            }

            ensureMainThread {
                if duration.isIndefinite {
                    completion(TimeInterval.infinity, asset)
                } else if abs(duration.seconds.distance(to: 0.0)) > tolerance {
                    let newItem = PlaylistInfo(name: item.name,
                                               src: item.src,
                                               pageSrc: item.pageSrc,
                                               pageTitle: item.pageTitle,
                                               mimeType: item.mimeType,
                                               duration: duration.seconds,
                                               detected: item.detected,
                                               dateAdded: item.dateAdded,
                                               tagId: item.tagId)

                    PlaylistItem.updateItem(newItem) {
                        completion(duration.seconds, asset)
                    }
                } else {
                    completion(duration.seconds, asset)
                }
            }
        }
        
        return PlaylistAssetFetcher(asset: asset)
    }
    
    func getAssetDurationFormatted(item: PlaylistInfo, _ completion: @escaping (String) -> Void) -> PlaylistAssetFetcher? {
        return getAssetDuration(item: item) { duration, asset in
            let domain = URL(string: item.pageSrc)?.baseDomain ?? "0s"
            if let duration = duration {
                if duration.isInfinite {
                    // Live video/audio
                    completion(Strings.PlayList.playlistLiveMediaStream)
                } else if abs(duration.distance(to: 0.0)) > 0.00001 {
                    completion(Self.formatter.string(from: duration) ?? domain)
                } else {
                    completion(domain)
                }
            } else {
                // Media Item is expired or some sort of error occurred retrieving its duration
                // Whatever the reason, we mark it as expired now
                completion(Strings.PlayList.expiredLabelTitle)
            }
        }
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        PlaylistManager.shared.numberOfAssets
    }
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        Constants.tableRowHeight
    }
    
    func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        Constants.tableHeaderHeight
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard let cell = tableView.dequeueReusableCell(withIdentifier: Constants.playListCellIdentifier, for: indexPath) as? PlaylistCell else {
            return UITableViewCell()
        }
        
        return cell
    }
    
    func tableView(_ tableView: UITableView, willDisplay cell: UITableViewCell, forRowAt indexPath: IndexPath) {
        guard let cell = cell as? PlaylistCell,
              let item = PlaylistManager.shared.itemAtIndex(indexPath.row) else {
            return
        }
        
        cell.prepareForDisplay()
        let domain = URL(string: item.pageSrc)?.baseDomain ?? "0s"
        
        cell.do {
            $0.selectionStyle = .none
            $0.titleLabel.text = item.name
            $0.detailLabel.text = domain
            $0.contentView.backgroundColor = .clear
            $0.backgroundColor = .clear
            $0.thumbnailView.image = nil
            $0.thumbnailView.backgroundColor = .black
        }
        
        let cacheState = PlaylistManager.shared.state(for: item.pageSrc)
        switch cacheState {
        case .inProgress:
            cell.durationFetcher = getAssetDurationFormatted(item: item) {
                cell.detailLabel.text = "\($0) - \(Strings.PlayList.savingForOfflineLabelTitle)"
            }
        case .downloaded:
            if let itemSize = PlaylistManager.shared.sizeOfDownloadedItem(for: item.pageSrc) {
                cell.durationFetcher = getAssetDurationFormatted(item: item) {
                    cell.detailLabel.text = "\($0) - \(itemSize)"
                }
            } else {
                cell.durationFetcher = getAssetDurationFormatted(item: item) {
                    cell.detailLabel.text = "\($0) - \(Strings.PlayList.savedForOfflineLabelTitle)"
                }
            }
        case .invalid:
            cell.durationFetcher = getAssetDurationFormatted(item: item) {
                cell.detailLabel.text = $0
            }
        }
        
        // Load the HLS/Media thumbnail. If it fails, fall-back to favIcon
        if let assetUrl = URL(string: item.src), let favIconUrl = URL(string: item.pageSrc) {
            cell.thumbnailActivityIndicator.startAnimating()
            cell.thumbnailGenerator.loadThumbnail(assetUrl: assetUrl, favIconUrl: favIconUrl) { [weak cell] image in
                guard let cell = cell else { return }
                
                cell.thumbnailView.image = image ?? FaviconFetcher.defaultFaviconImage
                cell.thumbnailView.backgroundColor = .black
                cell.thumbnailView.contentMode = .scaleAspectFit
                cell.thumbnailActivityIndicator.stopAnimating()
            }
        }
    }
    
    func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
        return UIView()
    }
}
