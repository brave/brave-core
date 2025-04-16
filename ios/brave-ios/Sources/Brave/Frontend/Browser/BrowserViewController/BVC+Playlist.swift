// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveUI
import Data
import Foundation
import Growth
import Onboarding
import Playlist
import PlaylistUI
import Preferences
import Shared
import UIKit
import Web
import os.log

extension BrowserViewController: PlaylistScriptHandlerDelegate,
  PlaylistFolderSharingScriptHandlerDelegate
{
  static var didShowStorageFullWarning = false
  func createPlaylistPopover(item: PlaylistInfo, tab: (any TabState)?) -> PopoverController {

    let folderName =
      PlaylistItem.getItem(uuid: item.tagId)?.playlistFolder?.title
      ?? Strings.Playlist.defaultPlaylistTitle

    return PopoverController(
      content: PlaylistPopoverView(folderName: folderName) { [weak self, weak tab] action in
        guard let self = self,
          let selectedTab = tab,
          let item = selectedTab.playlistItem
        else {
          return
        }
        // Dismiss popover
        UIImpactFeedbackGenerator(style: .medium).vibrate()

        self.dismiss(animated: true) {
          switch action {
          case .openPlaylist:
            DispatchQueue.main.async {
              if let tab {
                PlaylistScriptHandler.getCurrentTime(tab: tab, nodeTag: item.tagId) {
                  [weak self] currentTime in
                  self?.openPlaylist(tab: tab, item: item, playbackOffset: currentTime)
                }
              } else {
                self.openPlaylist(tab: tab, item: item, playbackOffset: 0.0)
              }
            }
          case .changeFolders:
            guard let item = PlaylistItem.getItem(uuid: item.tagId) else { return }
            let controller = PlaylistChangeFoldersViewController(item: item)
            self.present(controller, animated: true)
          case .timedOut:
            // Just dismisses
            break
          }
        }
      },
      autoLayoutConfiguration: .phoneWidth
    )
  }

  func updatePlaylistURLBar(
    tab: (any TabState)?,
    state: PlaylistItemAddedState,
    item: PlaylistInfo?
  ) {
    // `tab` is nil when closed, along with the `.none` state and nil `item`
    guard let tab = tab else { return }

    if tab === tabManager.selectedTab {
      tab.playlistItemState = state
      tab.playlistItem = item

      let shouldShowPlaylistURLBarButton =
        tab.visibleURL?.isPlaylistSupportedSiteURL == true
        && Preferences.Playlist.enablePlaylistURLBarButton.value

      let browsers = UIApplication.shared.connectedScenes.compactMap({ $0 as? UIWindowScene })
        .compactMap({ $0.browserViewController })
      browsers.forEach { browser in
        browser.openInPlayListActivity(info: state == .existingItem ? item : nil)
        browser.addToPlayListActivity(
          info: state == .newItem ? item : nil,
          itemDetected: state == .newItem
        )

        switch state {
        case .none:
          browser.topToolbar.updatePlaylistButtonState(.none)
          browser.topToolbar.menuButton.removeBadge(.playlist, animated: true)
          browser.toolbar?.menuButton.removeBadge(.playlist, animated: true)
        case .newItem:
          browser.topToolbar.updatePlaylistButtonState(
            shouldShowPlaylistURLBarButton ? .addToPlaylist : .none
          )
          if Preferences.Playlist.enablePlaylistMenuBadge.value {
            browser.topToolbar.menuButton.addBadge(.playlist, animated: true)
            browser.toolbar?.menuButton.addBadge(.playlist, animated: true)
          } else {
            browser.topToolbar.menuButton.removeBadge(.playlist, animated: true)
            browser.toolbar?.menuButton.removeBadge(.playlist, animated: true)
          }
        case .existingItem:
          browser.topToolbar.updatePlaylistButtonState(
            shouldShowPlaylistURLBarButton ? .addedToPlaylist(item) : .none
          )
          browser.topToolbar.menuButton.removeBadge(.playlist, animated: true)
          browser.toolbar?.menuButton.removeBadge(.playlist, animated: true)
        }
      }
    }
  }

  func showPlaylistPopover(tab: (any TabState)?) {
  }

  func showPlaylistToast(tab: (any TabState)?, state: PlaylistItemAddedState, item: PlaylistInfo?) {
    updatePlaylistURLBar(tab: tab, state: state, item: item)

    guard let selectedTab = tabManager.selectedTab,
      selectedTab === tab,
      selectedTab.visibleURL?.isPlaylistSupportedSiteURL == true
    else {
      return
    }

    if let toast = pendingToast as? PlaylistToast {
      toast.item = item
      return
    }

    pendingToast = PlaylistToast(
      item: item,
      state: state,
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
                UIImpactFeedbackGenerator(style: .medium).vibrate()
              }
            }
          } else {
            self.pendingToast = nil
          }

        // Item already exists in playlist, so ask them if they want to view it there
        // Item was added to playlist by the user, so ask them if they want to view it there
        case .newItem, .existingItem:
          if buttonPressed {
            UIImpactFeedbackGenerator(style: .medium).vibrate()

            DispatchQueue.main.async {
              if let tab {
                PlaylistScriptHandler.getCurrentTime(tab: tab, nodeTag: item.tagId) {
                  [weak self] currentTime in
                  self?.openPlaylist(tab: tab, item: item, playbackOffset: currentTime)
                }
              } else {
                self.openPlaylist(tab: tab, item: item, playbackOffset: 0.0)
              }
            }
          }

          self.pendingToast = nil
        }
      }
    )

    if let pendingToast = pendingToast {
      let duration = state == .none ? 10 : 5
      show(toast: pendingToast, afterWaiting: .milliseconds(250), duration: .seconds(duration))
    }
  }

  func showPlaylistAlert(tab: (any TabState)?, state: PlaylistItemAddedState, item: PlaylistInfo?) {
    // Has to be done otherwise it is impossible to play a video after selecting its elements
    UIMenuController.shared.hideMenu()

    let style: UIAlertController.Style =
      UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
    let alert = UIAlertController(
      title: Strings.PlayList.addToPlayListAlertTitle,
      message: Strings.PlayList.addToPlayListAlertDescription,
      preferredStyle: style
    )

    alert.addAction(
      UIAlertAction(
        title: Strings.PlayList.addToPlayListAlertTitle,
        style: .default,
        handler: { _ in
          // Update playlist with new items..

          guard let item = item else { return }
          self.addToPlaylist(item: item) { [weak self] addedToPlaylist in
            guard let self = self else { return }

            UIImpactFeedbackGenerator(style: .medium).vibrate()

            if addedToPlaylist {
              self.showPlaylistToast(tab: tab, state: .existingItem, item: item)
            }
          }
        }
      )
    )
    alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
    present(alert, animated: true, completion: nil)
  }

  func showPlaylistOnboarding(tab: (any TabState)?) {
    // Do NOT show the playlist onboarding popup if the tab isn't visible

    guard Preferences.Playlist.enablePlaylistURLBarButton.value,
      let selectedTab = tabManager.selectedTab,
      selectedTab === tab,
      selectedTab.playlistItemState != .none
    else {
      return
    }

    let shouldShowOnboarding = tab?.visibleURL?.isPlaylistSupportedSiteURL == true

    if shouldShowOnboarding {
      if Preferences.Playlist.addToPlaylistURLBarOnboardingCount.value < 2,
        shouldShowPlaylistOnboardingThisSession,
        presentedViewController == nil
      {
        Preferences.Playlist.addToPlaylistURLBarOnboardingCount.value += 1

        topToolbar.layoutIfNeeded()
        view.layoutIfNeeded()

        // Ensure url bar is expanded before presenting a popover on it
        toolbarVisibilityViewModel.toolbarState = .expanded

        DispatchQueue.main.async {
          let model = OnboardingPlaylistModel()
          let popover = PopoverController(content: OnboardingPlaylistView(model: model))
          popover.previewForOrigin = .init(
            view: self.topToolbar.locationView.playlistButton,
            action: { [weak tab] popover in
              guard let item = tab?.playlistItem else {
                popover.dismissPopover()
                return
              }
              popover.previewForOrigin = nil
              self.addToPlaylist(item: item) { didAddItem in
                let folderName = PlaylistItem.getItem(uuid: item.tagId)?.playlistFolder?.title ?? ""
                model.step = .completed(folderName: folderName)
              }
            }
          )
          popover.present(from: self.topToolbar.locationView.playlistButton, on: self)

          model.onboardingCompleted = { [weak tab, weak popover] in
            popover?.dismissPopover()
            self.openPlaylist(tab: tab, item: tab?.playlistItem)
          }
        }

        shouldShowPlaylistOnboardingThisSession = false
      }
    }
  }

  func openPlaylist(tab: (any TabState)?, item: PlaylistInfo?, folderSharingPageUrl: String? = nil)
  {
    if let item, let tab {
      PlaylistScriptHandler.getCurrentTime(tab: tab, nodeTag: item.tagId) {
        [weak self] currentTime in
        self?.openPlaylist(
          tab: tab,
          item: item,
          playbackOffset: currentTime,
          folderSharingPageUrl: folderSharingPageUrl
        )
      }
    } else {
      openPlaylist(
        tab: tab,
        item: item,
        playbackOffset: 0.0,
        folderSharingPageUrl: folderSharingPageUrl
      )
    }
  }

  private func openPlaylist(
    tab: (any TabState)?,
    item: PlaylistInfo?,
    playbackOffset: Double,
    folderSharingPageUrl: String? = nil
  ) {
    let playlistController = PlaylistCoordinator.shared.getPlaylistController(
      tab: tab,
      initialItem: item,
      initialItemPlaybackOffset: playbackOffset
    )
    // Legacy UI set up for shared playlists
    if let playlistController = playlistController as? PlaylistViewController {
      if let folderSharingPageUrl = folderSharingPageUrl {
        playlistController.setFolderSharingUrl(folderSharingPageUrl)
      }
    }

    // Donate Open Playlist Activity for suggestions
    let openPlaylist = ActivityShortcutManager.shared.createShortcutActivity(type: .openPlayList)
    self.userActivity = openPlaylist
    openPlaylist.becomeCurrent()
    PlaylistP3A.recordUsage()

    present(playlistController, animated: true) {
      // Legacy UI set up for shared playlists
      if let playlistController = playlistController as? PlaylistViewController {
        if let folderSharingPageUrl = folderSharingPageUrl {
          playlistController.setFolderSharingUrl(folderSharingPageUrl)
        }
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

  func addToPlaylist(
    item: PlaylistInfo,
    folderUUID: String? = nil,
    completion: ((_ didAddItem: Bool) -> Void)? = nil
  ) {
    PlaylistP3A.recordUsage()

    let addItemToPlaylist = {
      [weak self] (
        item: PlaylistInfo,
        folderUUID: String?,
        completion: ((_ didAddItem: Bool) -> Void)?
      ) in
      PlaylistItem.addItem(item, folderUUID: folderUUID, cachedData: nil) {
        guard let self = self else { return }

        if let url = URL(string: item.src), url.scheme == "blob" {
          // Spawn a WebView to load the non-blob asset
          Task { @MainActor in
            let mediaStreamer = PlaylistMediaStreamer(
              playerView: self.view,
              webLoaderFactory: LivePlaylistWebLoaderFactory()
            )

            let newItem = try await mediaStreamer.loadMediaStreamingAsset(item)
            PlaylistManager.shared.autoDownload(item: newItem)
          }
        } else {
          PlaylistManager.shared.autoDownload(item: item)
        }

        self.updatePlaylistURLBar(
          tab: self.tabManager.selectedTab,
          state: .existingItem,
          item: item
        )
        completion?(true)
      }
    }

    if PlaylistManager.shared.isDiskSpaceEncumbered()
      && !BrowserViewController.didShowStorageFullWarning
    {
      BrowserViewController.didShowStorageFullWarning = true
      let style: UIAlertController.Style =
        UIDevice.current.userInterfaceIdiom == .pad ? .alert : .actionSheet
      let alert = UIAlertController(
        title: Strings.PlayList.playlistDiskSpaceWarningTitle,
        message: Strings.PlayList.playlistDiskSpaceWarningMessage,
        preferredStyle: style
      )

      alert.addAction(
        UIAlertAction(
          title: Strings.OKString,
          style: .default,
          handler: { [weak self] _ in
            guard let self = self else { return }
            self.openInPlaylistActivityItem = (enabled: true, item: item)
            self.addToPlayListActivityItem = nil

            AppReviewManager.shared.processSubCriteria(for: .numberOfPlaylistItems)
            addItemToPlaylist(item, folderUUID, completion)
          }
        )
      )

      alert.addAction(
        UIAlertAction(
          title: Strings.cancelButtonTitle,
          style: .cancel,
          handler: { _ in
            completion?(false)
          }
        )
      )

      // Sometimes the MENU controller is being displayed and cannot present the alert
      // So we need to ask it to present the alert
      (presentedViewController ?? self).present(alert, animated: true, completion: nil)
    } else {
      openInPlaylistActivityItem = (enabled: true, item: item)
      addToPlayListActivityItem = nil

      AppReviewManager.shared.processSubCriteria(for: .numberOfPlaylistItems)
      addItemToPlaylist(item, folderUUID, completion)
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

      BrowserViewController.playlistSyncFoldersTimer = Timer(
        fire: lastSyncDate,
        interval: 4.hours,
        repeats: true,
        block: { _ in
          Preferences.Playlist.lastPlaylistFoldersSyncTime.value = Date()

          Task {
            try await PlaylistManager.syncSharedFolders()
          }
        }
      )
    }
  }
}
