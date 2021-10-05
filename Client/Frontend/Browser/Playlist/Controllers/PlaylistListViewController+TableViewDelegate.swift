// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveShared
import BraveUI
import Shared
import Data
import MediaPlayer

private let log = Logger.browserLogger

// MARK: UITableViewDelegate

extension PlaylistListViewController: UITableViewDelegate {
    
    private func shareItem(_ item: PlaylistInfo, anchorView: UIView?) {
        guard let url = URL(string: item.pageSrc) else {
            return
        }
        
        let activityViewController = UIActivityViewController(activityItems: [url],
                                                              applicationActivities: nil)
        
        activityViewController.excludedActivityTypes = [.openInIBooks, .saveToCameraRoll, .assignToContact]
        if UIDevice.current.userInterfaceIdiom == .pad {
            activityViewController.popoverPresentationController?.sourceView = anchorView ?? self.view
        }
        self.present(activityViewController, animated: true, completion: nil)
    }
    
    func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
        
        if indexPath.row < 0 || indexPath.row >= PlaylistManager.shared.numberOfAssets {
            return nil
        }

        guard let currentItem = PlaylistManager.shared.itemAtIndex(indexPath.row) else {
            return nil
        }
        
        let cacheState = PlaylistManager.shared.state(for: currentItem.pageSrc)
        
        let cacheAction = UIContextualAction(style: .normal, title: nil, handler: { [weak self] (action, view, completionHandler) in
            guard let self = self else { return }
            
            switch cacheState {
                case .inProgress:
                    PlaylistManager.shared.cancelDownload(item: currentItem)
                    tableView.reloadRows(at: [indexPath], with: .automatic)
                case .invalid:
                    if PlaylistManager.shared.isDiskSpaceEncumbered() {
                        let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
                        let alert = UIAlertController(
                            title: Strings.PlayList.playlistDiskSpaceWarningTitle, message: Strings.PlayList.playlistDiskSpaceWarningMessage, preferredStyle: style)
                        
                        alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: { _ in
                            PlaylistManager.shared.download(item: currentItem)
                            tableView.reloadRows(at: [indexPath], with: .automatic)
                        }))
                        
                        alert.addAction(UIAlertAction(title: Strings.CancelString, style: .cancel, handler: nil))
                        self.present(alert, animated: true, completion: nil)
                    } else {
                        PlaylistManager.shared.download(item: currentItem)
                        tableView.reloadRows(at: [indexPath], with: .automatic)
                    }
                case .downloaded:
                    let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
                    let alert = UIAlertController(
                        title: Strings.PlayList.removePlaylistOfflineDataAlertTitle, message: Strings.PlayList.removePlaylistOfflineDataAlertMessage, preferredStyle: style)
                    
                    alert.addAction(UIAlertAction(title: Strings.PlayList.removeActionButtonTitle, style: .destructive, handler: { _ in
                        _ = PlaylistManager.shared.deleteCache(item: currentItem)
                        tableView.reloadRows(at: [indexPath], with: .automatic)
                    }))
                    
                    alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
                    self.present(alert, animated: true, completion: nil)
            }
            
            completionHandler(true)
        })
        
        let deleteAction = UIContextualAction(style: .normal, title: nil, handler: { [weak self] (action, view, completionHandler) in
            guard let self = self else { return }
            
            let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
            let alert = UIAlertController(
                title: Strings.PlayList.removePlaylistVideoAlertTitle, message: Strings.PlayList.removePlaylistVideoAlertMessage, preferredStyle: style)
            
            alert.addAction(UIAlertAction(title: Strings.PlayList.removeActionButtonTitle, style: .destructive, handler: { [weak self] _ in
                guard let self = self else { return }
                
                self.delegate?.deleteItem(item: currentItem, at: indexPath.row)
                
                if self.delegate?.currentPlaylistItem == nil {
                    self.updateTableBackgroundView()
                    self.activityIndicator.stopAnimating()
                }
            }))
            
            alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
            self.present(alert, animated: true, completion: nil)
            
            completionHandler(true)
        })
        
        let shareAction = UIContextualAction(style: .normal, title: nil, handler: { [weak self] (action, view, completionHandler) in
            guard let self = self else { return }
            
            let isPrivateBrowsing = PrivateBrowsingManager.shared.isPrivateBrowsing
            let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
            
            let alert = UIAlertController(
                title: currentItem.pageTitle,
                message: nil,
                preferredStyle: style)
            
            // If we're already in private browsing, this should not show
            // Option to open in regular tab
            if !isPrivateBrowsing {
                alert.addAction(UIAlertAction(title: Strings.PlayList.sharePlaylistOpenInNewTabTitle, style: .default, handler: { [weak self] _ in
                    guard let self = self else { return }
                                
                    if let browser = PlaylistCarplayManager.shared.browserController,
                       let pageURL = URL(string: currentItem.pageSrc) {
                        
                        self.dismiss(animated: true) {
                            browser.tabManager.addTabAndSelect(URLRequest(url: pageURL),
                                                               isPrivate: false)
                        }
                    }
                }))
            }
            
            // Option to open in private browsing tab
            alert.addAction(UIAlertAction(title: Strings.PlayList.sharePlaylistOpenInNewPrivateTabTitle, style: .default, handler: { [weak self] _ in
                guard let self = self else { return }
                            
                if let browser = PlaylistCarplayManager.shared.browserController,
                   let pageURL = URL(string: currentItem.pageSrc) {
                    
                    self.dismiss(animated: true) {
                        browser.tabManager.addTabAndSelect(URLRequest(url: pageURL),
                                                           isPrivate: true)
                    }
                }
            }))
            
            alert.addAction(UIAlertAction(title: Strings.PlayList.sharePlaylistShareActionMenuTitle, style: .default, handler: { [weak self] _ in
                self?.shareItem(currentItem, anchorView: tableView.cellForRow(at: indexPath))
            }))
            
            alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
            self.present(alert, animated: true, completion: nil)
            
            completionHandler(true)
        })

        cacheAction.image = cacheState == .invalid ? #imageLiteral(resourceName: "playlist_download") : #imageLiteral(resourceName: "playlist_delete_download")
        cacheAction.backgroundColor = UIColor.braveDarkerBlurple
        
        deleteAction.image = #imageLiteral(resourceName: "playlist_delete_item")
        deleteAction.backgroundColor = UIColor.braveErrorLabel
        
        shareAction.image = UIImage(systemName: "square.and.arrow.up")
        shareAction.backgroundColor = UIColor.braveInfoLabel
        
        return UISwipeActionsConfiguration(actions: [deleteAction, shareAction, cacheAction])
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        if tableView.isEditing {
            tableView.setEditing(false, animated: true)
            return
        }
        
        prepareToPlayItem(at: indexPath) { [weak self] item in
            guard let item = item else {
                self?.activityIndicator.stopAnimating()
                return
            }
            
            self?.delegate?.playItem(item: item) { [weak self] error in
                guard let self = self else {
                    PlaylistCarplayManager.shared.currentPlaylistItem = nil
                    PlaylistCarplayManager.shared.currentlyPlayingItemIndex = -1
                    return
                }
                self.activityIndicator.stopAnimating()
                
                switch error {
                case .other(let err):
                    log.error(err)
                    self.commitPlayerItemTransaction(at: indexPath, isExpired: false)
                    self.delegate?.displayLoadingResourceError()
                case .expired:
                    self.commitPlayerItemTransaction(at: indexPath, isExpired: true)
                    self.delegate?.displayExpiredResourceError(item: item)
                case .none:
                    PlaylistCarplayManager.shared.currentlyPlayingItemIndex = indexPath.row
                    PlaylistCarplayManager.shared.currentPlaylistItem = item
                    self.commitPlayerItemTransaction(at: indexPath, isExpired: false)
                    self.delegate?.updateLastPlayedItem(item: item)
                case .cancelled:
                    self.commitPlayerItemTransaction(at: indexPath, isExpired: false)
                    log.debug("User cancelled Playlist Playback")
                }
            }
        }
    }
}
