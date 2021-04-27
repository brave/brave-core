// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import Shared
import BraveShared

private let log = Logger.browserLogger

extension BrowserViewController: PlaylistHelperDelegate {
    func showPlaylistAlert(_ alertController: UIAlertController) {
        self.present(alertController, animated: true)
    }
    
    func showPlaylistToast(info: PlaylistInfo, itemState: PlaylistItemAddedState) {
        guard Preferences.Playlist.showToastForAdd.value,
              let selectedTab = tabManager.selectedTab,
              selectedTab.url?.isPlaylistSupportedSiteURL == true else {
            return
        }
        
        if let toast = playlistToast {
            toast.item = info
            return
        }
        
        // Item requires the user to choose whether or not to add it to playlists
        let toast = PlaylistToast(item: info, state: itemState, completion: { [weak self] buttonPressed in
            guard let self = self, let item = self.playlistToast?.item else { return }
            
            switch itemState {
            // Item requires user action to add it to playlists
            case .pendingUserAction:
                if buttonPressed {
                    // Update playlist with new items..
                    self.addToPlaylist(item: item) { [weak self] didAddItem in
                        guard let self = self else { return }
                        
                        log.debug("Playlist Item Added")
                        self.playlistToast = nil
                        
                        if didAddItem {
                            self.showPlaylistToast(info: item, itemState: .added)
                            UIImpactFeedbackGenerator(style: .medium).bzzt()
                        }
                    }
                } else {
                    self.playlistToast = nil
                }
                
            // Item already exists in playlist, so ask them if they want to view it there
            // Item was added to playlist by the user, so ask them if they want to view it there
            case .added, .existing:
                if buttonPressed {
                    self.openPlaylist()
                    UIImpactFeedbackGenerator(style: .medium).bzzt()
                }
                
                self.playlistToast = nil
            }
        })
        
        playlistToast = toast
        let duration = itemState == .pendingUserAction ? 10 : 5
        show(toast: toast, afterWaiting: .milliseconds(250), duration: .seconds(duration))
    }
    
    func dismissPlaylistToast(animated: Bool) {
        playlistToast?.dismiss(false, animated: animated)
    }
    
    private func openPlaylist() {
        let playlistController = (UIApplication.shared.delegate as? AppDelegate)?.playlistRestorationController ?? PlaylistViewController()
        playlistController.modalPresentationStyle = .fullScreen
        present(playlistController, animated: true)
    }
    
    func addToPlayListActivity(info: PlaylistInfo?, itemDetected: Bool) {
        if info == nil {
            addToPlayListActivityItem = nil
        } else {
            addToPlayListActivityItem = (enabled: itemDetected, item: info)
        }
    }
    
    func openInPlayListActivity(info: PlaylistInfo?) {
        if info == nil {
            openInPlaylistActivityItem = nil
        } else {
            openInPlaylistActivityItem = (enabled: true, item: info)
        }
    }
    
    func addToPlaylist(item: PlaylistInfo, completion: ((_ didAddItem: Bool) -> Void)?) {
        if PlaylistManager.shared.isDiskSpaceEncumbered() {
            let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
            let alert = UIAlertController(
                title: Strings.PlayList.playlistDiskSpaceWarningTitle, message: Strings.PlayList.playlistDiskSpaceWarningMessage, preferredStyle: style)
            
            alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: { [weak self] _ in
                guard let self = self else { return }
                self.openInPlaylistActivityItem = (enabled: true, item: item)
                self.addToPlayListActivityItem = nil
                
                PlaylistItem.addItem(item, cachedData: nil) {
                    PlaylistManager.shared.autoDownload(item: item)
                    completion?(true)
                }
            }))
            
            alert.addAction(UIAlertAction(title: Strings.CancelString, style: .cancel, handler: { _ in
                completion?(false)
            }))
            self.present(alert, animated: true, completion: nil)
        } else {
            openInPlaylistActivityItem = (enabled: true, item: item)
            addToPlayListActivityItem = nil
            
            PlaylistItem.addItem(item, cachedData: nil) {
                PlaylistManager.shared.autoDownload(item: item)
                completion?(true)
            }
        }
    }
    
    func openInPlaylist(item: PlaylistInfo, completion: (() -> Void)?) {
        let playlistController = (UIApplication.shared.delegate as? AppDelegate)?.playlistRestorationController ?? PlaylistViewController()
        playlistController.modalPresentationStyle = .fullScreen
        present(playlistController, animated: true) {
            completion?()
        }
    }
}
