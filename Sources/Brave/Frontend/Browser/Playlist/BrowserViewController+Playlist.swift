// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import Shared
import BraveShared
import Preferences
import BraveUI
import UIKit
import Growth
import os.log
import Onboarding

extension BrowserViewController: PlaylistScriptHandlerDelegate, PlaylistFolderSharingScriptHandlerDelegate {

  private func createPlaylistPopover(tab: Tab?, state: PlaylistPopoverState) -> PopoverController {
    return PopoverController(
      contentController: PlaylistPopoverViewController(state: state).then {
        $0.rootView.onPrimaryButtonPressed = { [weak self, weak tab] in
          guard let self = self,
            let selectedTab = tab,
            let item = selectedTab.playlistItem
          else { return }

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
                  PlaylistScriptHandler.getCurrentTime(webView: webView, nodeTag: item.tagId) { [weak self] currentTime in
                    self?.openPlaylist(tab: tab, item: item, playbackOffset: currentTime)
                  }
                } else {
                  self.openPlaylist(tab: tab, item: item, playbackOffset: 0.0)
                }
              }
            }
          }
        }

        $0.rootView.onSecondaryButtonPressed = { [weak tab] in
          guard let selectedTab = tab,
            let item = selectedTab.playlistItem
          else { return }
          UIImpactFeedbackGenerator(style: .medium).bzzt()

          self.dismiss(animated: true)

          DispatchQueue.main.async {
            if PlaylistManager.shared.delete(item: item) {
              self.updatePlaylistURLBar(tab: tab, state: .newItem, item: item)
            }
          }
        }
      })
  }

  func updatePlaylistURLBar(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?) {
    // `tab` is nil when closed, along with the `.none` state and nil `item`
    guard let tab = tab else { return }

    if tab === tabManager.selectedTab {
      openInPlayListActivity(info: state == .existingItem ? item : nil)
      addToPlayListActivity(info: state == .newItem ? item : nil, itemDetected: state == .newItem)

      tab.playlistItemState = state
      tab.playlistItem = item

      let shouldShowPlaylistURLBarButton = tab.url?.isPlaylistSupportedSiteURL == true && Preferences.Playlist.enablePlaylistURLBarButton.value

      switch state {
      case .none:
        topToolbar.updatePlaylistButtonState(.none)
        topToolbar.menuButton.removeBadge(.playlist, animated: true)
        toolbar?.menuButton.removeBadge(.playlist, animated: true)
      case .newItem:
        topToolbar.updatePlaylistButtonState(shouldShowPlaylistURLBarButton ? .addToPlaylist : .none)
        if Preferences.Playlist.enablePlaylistMenuBadge.value {
          topToolbar.menuButton.addBadge(.playlist, animated: true)
          toolbar?.menuButton.addBadge(.playlist, animated: true)
        } else {
          topToolbar.menuButton.removeBadge(.playlist, animated: true)
          toolbar?.menuButton.removeBadge(.playlist, animated: true)
        }
      case .existingItem:
        topToolbar.updatePlaylistButtonState(shouldShowPlaylistURLBarButton ? .addedToPlaylist : .none)
        topToolbar.menuButton.removeBadge(.playlist, animated: true)
        toolbar?.menuButton.removeBadge(.playlist, animated: true)
      }
    }
  }

  func showPlaylistPopover(tab: Tab?, state: PlaylistPopoverState) {
    guard let selectedTab = tabManager.selectedTab,
      tab == selectedTab,
      let playlistItem = selectedTab.playlistItem
    else {
      return
    }

    if state == .addToPlaylist {
      UIImpactFeedbackGenerator(style: .medium).bzzt()

      // Update playlist with new items.
      self.addToPlaylist(item: playlistItem) { [weak self] didAddItem in
        guard let self = self else { return }

        if didAddItem {
          self.updatePlaylistURLBar(tab: tab, state: .existingItem, item: playlistItem)

          DispatchQueue.main.async {
            self.showPlaylistPopover(tab: tab, state: .addedToPlaylist)
          }
        }
      }
      return
    }

    let popover = createPlaylistPopover(tab: tab, state: state)
    popover.present(from: topToolbar.locationView.playlistButton, on: self)
  }

  func showPlaylistToast(tab: Tab?, state: PlaylistItemAddedState, item: PlaylistInfo?) {
    updatePlaylistURLBar(tab: tab, state: state, item: item)

    guard let selectedTab = tabManager.selectedTab,
      selectedTab === tab,
      selectedTab.url?.isPlaylistSupportedSiteURL == true
    else {
      return
    }

    if let toast = pendingToast as? PlaylistToast {
      toast.item = item
      return
    }

    pendingToast = PlaylistToast(
      item: item, state: state,
      completion: { [weak self] buttonPressed in
        guard let self = self,
          let item = (self.pendingToast as? PlaylistToast)?.item
        else { return }

        switch state {
        // Item requires user action to add it to playlists
        case .none:
          if buttonPressed {
            // Update playlist with new items..
            self.addToPlaylist(item: item) { [weak self] didAddItem in
              guard let self = self else { return }

              Logger.module.debug("Playlist Item Added")
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
                PlaylistScriptHandler.getCurrentTime(webView: webView, nodeTag: item.tagId) { [weak self] currentTime in
                  self?.openPlaylist(tab: tab, item: item, playbackOffset: currentTime)
                }
              } else {
                self.openPlaylist(tab: tab, item: item, playbackOffset: 0.0)
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

    alert.addAction(
      UIAlertAction(
        title: Strings.PlayList.addToPlayListAlertTitle, style: .default,
        handler: { _ in
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
    // Do NOT show the playlist onboarding popup if the tab isn't visible

    guard Preferences.Playlist.enablePlaylistURLBarButton.value,
      let selectedTab = tabManager.selectedTab,
      selectedTab === tab,
      selectedTab.playlistItemState != .none
    else {
      return
    }

    let shouldShowOnboarding = tab?.url?.isPlaylistSupportedSiteURL == true

    if shouldShowOnboarding {
      if Preferences.Playlist.addToPlaylistURLBarOnboardingCount.value < 2,
        shouldShowPlaylistOnboardingThisSession,
        presentedViewController == nil {
        Preferences.Playlist.addToPlaylistURLBarOnboardingCount.value += 1

        topToolbar.layoutIfNeeded()
        view.layoutIfNeeded()

        DispatchQueue.main.async {
          let onboardingController = PlaylistOnboardingViewController()
          let popover = PopoverController(contentController: onboardingController)
          popover.present(from: self.topToolbar.locationView.playlistButton, on: self)

          let pulseAnimation = RadialPulsingAnimation(ringCount: 3)
          pulseAnimation.present(
            icon: self.topToolbar.locationView.playlistButton.snapshot,
            from: self.topToolbar.locationView.playlistButton,
            on: popover,
            controller: self)
          pulseAnimation.frame = pulseAnimation.frame.insetBy(dx: 10.0, dy: 12.0)
          
          pulseAnimation.animationViewPressed = {
            popover.dismissPopover()
          }
          
          popover.popoverDidDismiss = { _ in
            pulseAnimation.removeFromSuperview()
          }

          onboardingController.rootView.onButtonPressed = { [weak self, unowned popover] in
            guard let self = self else {
              popover.dismiss(animated: true) {
                pulseAnimation.removeFromSuperview()
              }
              return
            }

            let isPrivate = PrivateBrowsingManager.shared.isPrivateBrowsing
            self.topToolbar.leaveOverlayMode()
            let tab = self.tabManager.addTab(
              PrivilegedRequest(url: .brave.playlist) as URLRequest,
              afterTab: self.tabManager.selectedTab,
              isPrivate: isPrivate)
            self.tabManager.selectTab(tab)

            popover.dismiss(animated: true) {
              pulseAnimation.removeFromSuperview()
            }
          }
        }

        shouldShowPlaylistOnboardingThisSession = false
      }
    }
  }

  func openPlaylist(tab: Tab?, item: PlaylistInfo?, playbackOffset: Double, folderSharingPageUrl: String? = nil) {
    let playlistController = PlaylistCarplayManager.shared.getPlaylistController(tab: tab,
                                                                                 initialItem: item,
                                                                                 initialItemPlaybackOffset: playbackOffset)
    playlistController.modalPresentationStyle = .fullScreen
    if let folderSharingPageUrl = folderSharingPageUrl {
      playlistController.setFolderSharingUrl(folderSharingPageUrl)
    }

    // Donate Open Playlist Activity for suggestions
    let openPlaylist = ActivityShortcutManager.shared.createShortcutActivity(type: .openPlayList)
    self.userActivity = openPlaylist
    openPlaylist.becomeCurrent()

    present(playlistController, animated: true) {
      if let folderSharingPageUrl = folderSharingPageUrl {
        playlistController.setFolderSharingUrl(folderSharingPageUrl)
      }
    }
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

      alert.addAction(
        UIAlertAction(
          title: Strings.OKString, style: .default,
          handler: { [weak self] _ in
            guard let self = self else { return }
            self.openInPlaylistActivityItem = (enabled: true, item: item)
            self.addToPlayListActivityItem = nil

            AppReviewManager.shared.processSubCriteria(for: .numberOfPlaylistItems)
            PlaylistItem.addItem(item, cachedData: nil) { [weak self] in
              guard let self = self else { return }
              PlaylistManager.shared.autoDownload(item: item)

              self.updatePlaylistURLBar(
                tab: self.tabManager.selectedTab,
                state: .existingItem,
                item: item)

              completion?(true)
            }
          }))

      alert.addAction(
        UIAlertAction(
          title: Strings.cancelButtonTitle, style: .cancel,
          handler: { _ in
            completion?(false)
          }))

      // Sometimes the MENU controller is being displayed and cannot present the alert
      // So we need to ask it to present the alert
      (presentedViewController ?? self).present(alert, animated: true, completion: nil)
    } else {
      openInPlaylistActivityItem = (enabled: true, item: item)
      addToPlayListActivityItem = nil

      AppReviewManager.shared.processSubCriteria(for: .numberOfPlaylistItems)
      PlaylistItem.addItem(item, cachedData: nil) { [weak self] in
        guard let self = self else { return }
        PlaylistManager.shared.autoDownload(item: item)

        self.updatePlaylistURLBar(
          tab: self.tabManager.selectedTab,
          state: .existingItem,
          item: item)
        completion?(true)
      }
    }
  }
  
  // MARK: - PlaylistFolderSharingHelperDelegate
  func openPlaylistSharingFolder(with pageUrl: String) {
    openPlaylist(tab: nil, item: nil, playbackOffset: 0.0, folderSharingPageUrl: pageUrl)
  }
}

extension BrowserViewController {
  private static var playlistSyncFoldersTimer: Timer?
  
  func openPlaylistSettingsMenu() {
    let playlistSettings = PlaylistSettingsViewController()
    let navigationController = UINavigationController(rootViewController: playlistSettings)
    self.present(navigationController, animated: true)
  }
  
  func syncPlaylistFolders() {
    if Preferences.Playlist.syncSharedFoldersAutomatically.value {
      BrowserViewController.playlistSyncFoldersTimer?.invalidate()
      
      let lastSyncDate = Preferences.Playlist.lastPlaylistFoldersSyncTime.value ?? Date()
      
      BrowserViewController.playlistSyncFoldersTimer = Timer(fire: lastSyncDate, interval: 4.hours, repeats: true, block: { _ in
        Preferences.Playlist.lastPlaylistFoldersSyncTime.value = Date()
        
        Task {
          try await PlaylistManager.syncSharedFolders()
        }
      })
    }
  }
}
