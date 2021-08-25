// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import Shared
import BraveShared
import BraveUI

private let log = Logger.browserLogger

extension BrowserViewController: PlaylistHelperDelegate {
    
    func updatePlaylistURLBar(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?) {
        // `tab` is nil when closed, along with the `.none` state and nil `item`
        guard let tab = tab else { return }
        
        if tab === tabManager.selectedTab {
            openInPlayListActivity(info: state == .existingItem ? item : nil)
            addToPlayListActivity(info: state == .newItem ? item : nil, itemDetected: state == .newItem)
            
            tab.playlistItemState = state
            tab.playlistItem = item
            
            let shouldShowPlaylistURLBarButton = tab.url?.isPlaylistSupportedSiteURL == true
            let playlistButton = topToolbar.locationView.playlistButton
            switch state {
            case .none:
                playlistButton.buttonState = .none
                topToolbar.menuButton.removeBadge(.playlist, animated: true)
                toolbar?.menuButton.removeBadge(.playlist, animated: true)
            case .newItem:
                playlistButton.buttonState = shouldShowPlaylistURLBarButton ? .addToPlaylist : .none
                topToolbar.menuButton.addBadge(.playlist, animated: true)
                toolbar?.menuButton.addBadge(.playlist, animated: true)
            case .existingItem:
                playlistButton.buttonState = shouldShowPlaylistURLBarButton ? .addedToPlaylist : .none
                topToolbar.menuButton.removeBadge(.playlist, animated: true)
                toolbar?.menuButton.removeBadge(.playlist, animated: true)
            }
        }
    }
    
    func showPlaylistPopover(tab: Tab?, state: PlaylistPopoverState) {
        guard Preferences.Playlist.showToastForAdd.value,
              let selectedTab = tabManager.selectedTab,
              tab == selectedTab else {
            return
        }
        
        if state == .addToPlaylist {
            if !shouldShowPlaylistOnboardingThisSession {
                if let item = selectedTab.playlistItem {
                    UIImpactFeedbackGenerator(style: .medium).bzzt()
                    
                    // Update playlist with new items.
                    self.addToPlaylist(item: item) { [weak self] didAddItem in
                        guard let self = self else { return }
                        
                        if didAddItem {
                            self.updatePlaylistURLBar(tab: tab, state: .existingItem, item: item)
                            
                            DispatchQueue.main.async {
                                self.showPlaylistPopover(tab: tab, state: .addedToPlaylist)
                            }
                        }
                    }
                }
                return
            }
        }
        
        let popover = PopoverController(contentController: PlaylistPopoverViewController(state: state).then {
            $0.rootView.onPrimaryButtonPressed = { [weak self] in
                guard let self = self,
                      let item = selectedTab.playlistItem else { return }
                
                switch state {
                case .addToPlaylist:
                    // Dismiss popover
                    UIImpactFeedbackGenerator(style: .medium).bzzt()
                    self.dismiss(animated: true, completion: nil)
                    
                    // Update playlist with new items.
                    self.addToPlaylist(item: item) { [weak self] didAddItem in
                        guard let self = self else { return }
                        
                        if didAddItem {
                            self.updatePlaylistURLBar(tab: tab, state: .existingItem, item: item)
                        }
                    }
                    
                case .addedToPlaylist:
                    // Dismiss popover
                    UIImpactFeedbackGenerator(style: .medium).bzzt()
                    
                    self.dismiss(animated: true) {
                        DispatchQueue.main.async {
                            if let webView = tab?.webView {
                                PlaylistHelper.getCurrentTime(webView: webView, nodeTag: item.tagId) { [weak self] currentTime in
                                    self?.openPlaylist(item: item, playbackOffset: currentTime)
                                }
                            } else {
                                self.openPlaylist(item: item, playbackOffset: 0.0)
                            }
                        }
                    }
                }
            }
            
            $0.rootView.onSecondaryButtonPressed = {
                guard let item = selectedTab.playlistItem else { return }
                UIImpactFeedbackGenerator(style: .medium).bzzt()
                
                self.dismiss(animated: true)
                
                DispatchQueue.main.async {
                    if PlaylistManager.shared.delete(item: item) {
                        self.updatePlaylistURLBar(tab: tab, state: .newItem, item: item)
                    }
                }
            }
        })
        popover.present(from: topToolbar.locationView.playlistButton, on: self)
    }
    
    func showPlaylistToast(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?) {
        updatePlaylistURLBar(tab: tab, state: state, item: item)
        
        guard let selectedTab = tabManager.selectedTab,
              selectedTab === tab,
              selectedTab.url?.isPlaylistSupportedSiteURL == true else {
            return
        }
        
        if let toast = pendingToast as? PlaylistToast {
            toast.item = item
            return
        }
        
        pendingToast = PlaylistToast(item: item, state: state, completion: { [weak self] buttonPressed in
            guard let self = self,
                  let item = (self.pendingToast as? PlaylistToast)?.item else { return }
            
            switch state {
            // Item requires user action to add it to playlists
            case .none:
                if buttonPressed {
                    // Update playlist with new items..
                    self.addToPlaylist(item: item) { [weak self] didAddItem in
                        guard let self = self else { return }
                        
                        log.debug("Playlist Item Added")
                        self.pendingToast = nil
                        
                        if didAddItem {
                            self.showPlaylistToast(tab: tab, state: .existingItem, item: item)
                            UIImpactFeedbackGenerator(style: .medium).bzzt()
                        }
                    }
                } else {
                    self.pendingToast = nil
                }
                
            // Item already exists in playlist, so ask them if they want to view it there
            // Item was added to playlist by the user, so ask them if they want to view it there
            case .newItem, .existingItem:
                if buttonPressed {
                    UIImpactFeedbackGenerator(style: .medium).bzzt()
                    
                    DispatchQueue.main.async {
                        if let webView = tab?.webView {
                            PlaylistHelper.getCurrentTime(webView: webView, nodeTag: item.tagId) { [weak self] currentTime in
                                self?.openPlaylist(item: item, playbackOffset: currentTime)
                            }
                        } else {
                            self.openPlaylist(item: item, playbackOffset: 0.0)
                        }
                    }
                }
                
                self.pendingToast = nil
            }
        })
        
        if let pendingToast = pendingToast {
            let duration = state == .none ? 10 : 5
            show(toast: pendingToast, afterWaiting: .milliseconds(250), duration: .seconds(duration))
        }
    }
    
    func showPlaylistAlert(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?) {
        // Has to be done otherwise it is impossible to play a video after selecting its elements
        UIMenuController.shared.hideMenu()
        
        let style: UIAlertController.Style = UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
        let alert = UIAlertController(
            title: Strings.PlayList.addToPlayListAlertTitle, message: Strings.PlayList.addToPlayListAlertDescription, preferredStyle: style)
        
        alert.addAction(UIAlertAction(title: Strings.PlayList.addToPlayListAlertTitle, style: .default, handler: { _ in
            // Update playlist with new items..
            
            guard let item = item else { return }
            self.addToPlaylist(item: item) { [weak self] addedToPlaylist in
                guard let self = self else { return }
                
                UIImpactFeedbackGenerator(style: .medium).bzzt()
                
                if addedToPlaylist {
                    self.showPlaylistToast(tab: tab, state: .existingItem, item: item)
                }
            }
        }))
        alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
        present(alert, animated: true, completion: nil)
    }
    
    func showPlaylistOnboarding(tab: Tab?) {
        let shouldShowOnboarding = tab?.url?.isPlaylistSupportedSiteURL == true
        
        if shouldShowOnboarding {
            if Preferences.Playlist.addToPlaylistURLBarOnboardingCount.value < 2 && shouldShowPlaylistOnboardingThisSession {
                Preferences.Playlist.addToPlaylistURLBarOnboardingCount.value += 1
                showPlaylistPopover(tab: tab, state: .addToPlaylist)
            }
            
            shouldShowPlaylistOnboardingThisSession = false
        }
    }
    
    func openPlaylist(item: PlaylistInfo?, playbackOffset: Double) {
        let playlistController = (UIApplication.shared.delegate as? AppDelegate)?.playlistRestorationController as? PlaylistViewController ?? PlaylistViewController(initialItem: item, initialItemPlaybackOffset: playbackOffset)
        playlistController.modalPresentationStyle = .fullScreen
        
        /// Donate Open Playlist Activity for suggestions
        let openPlaylist = ActivityShortcutManager.shared.createShortcutActivity(type: .openPlayList)
        self.userActivity = openPlaylist
        openPlaylist.becomeCurrent()

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
                
                PlaylistItem.addItem(item, cachedData: nil) { [weak self] in
                    guard let self = self else { return }
                    PlaylistManager.shared.autoDownload(item: item)
                    
                    self.updatePlaylistURLBar(tab: self.tabManager.selectedTab,
                                              state: .existingItem,
                                              item: item)
                    
                    completion?(true)
                }
            }))
            
            alert.addAction(UIAlertAction(title: Strings.CancelString, style: .cancel, handler: { _ in
                completion?(false)
            }))
            
            // Sometimes the MENU controller is being displayed and cannot present the alert
            // So we need to ask it to present the alert
            (presentedViewController ?? self).present(alert, animated: true, completion: nil)
        } else {
            openInPlaylistActivityItem = (enabled: true, item: item)
            addToPlayListActivityItem = nil
            
            PlaylistItem.addItem(item, cachedData: nil) { [weak self] in
                guard let self = self else { return }
                PlaylistManager.shared.autoDownload(item: item)
                
                self.updatePlaylistURLBar(tab: self.tabManager.selectedTab,
                                          state: .existingItem,
                                          item: item)
                completion?(true)
            }
        }
    }
}
