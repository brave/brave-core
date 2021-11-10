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
    
    func getAssetDurationFormatted(item: PlaylistInfo, _ completion: @escaping (String) -> Void) {
        PlaylistManager.shared.getAssetDuration(item: item) { duration in
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
        self.updateCellDownloadStatus(indexPath: indexPath,
                                      cell: cell,
                                      state: cacheState,
                                      percentComplete: nil)
        
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
