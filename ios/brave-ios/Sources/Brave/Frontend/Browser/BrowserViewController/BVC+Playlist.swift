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

extension BrowserViewController: PlaylistTabHelperDelegate {
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
              if let tab, let playlist = tab.playlist {
                playlist.getCurrentTime(nodeTag: item.tagId) {
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
        case .newItem:
          browser.topToolbar.updatePlaylistButtonState(
            shouldShowPlaylistURLBarButton ? .addToPlaylist : .none
          )
        case .existingItem:
          browser.topToolbar.updatePlaylistButtonState(
            shouldShowPlaylistURLBarButton ? .addedToPlaylist(item) : .none
          )
        }
      }
    }
  }

  func showPlaylistAlert(tab: (any TabState)?, state: PlaylistItemAddedState, item: PlaylistInfo?) {
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
              DispatchQueue.main.async { [self] in
                self.updatePlaylistURLBar(tab: tab, state: .existingItem, item: item)

                let popover = self.createPlaylistPopover(item: item, tab: tab)
                popover.present(from: self.topToolbar.locationView.playlistButton, on: self)
              }
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
      selectedTab.playlistItemState != PlaylistItemAddedState.none
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

  func openPlaylist(tab: (any TabState)?, item: PlaylistInfo?) {
    if !profileController.profile.prefs.isPlaylistAvailable {
      return
    }
    if let item, let tab, let playlist = tab.playlist {
      playlist.getCurrentTime(nodeTag: item.tagId) {
        [weak self] currentTime in
        self?.openPlaylist(
          tab: tab,
          item: item,
          playbackOffset: currentTime
        )
      }
    } else {
      openPlaylist(
        tab: tab,
        item: item,
        playbackOffset: 0.0
      )
    }
  }

  private func openPlaylist(
    tab: (any TabState)?,
    item: PlaylistInfo?,
    playbackOffset: Double
  ) {
    if !profileController.profile.prefs.isPlaylistAvailable {
      return
    }
    let playlistController = PlaylistCoordinator.shared.getPlaylistController(
      tab: tab,
      profile: profileController.profile,
      initialItem: item,
      initialItemPlaybackOffset: playbackOffset
    )

    // Donate Open Playlist Activity for suggestions
    let openPlaylist = ActivityShortcutManager.shared.createShortcutActivity(type: .openPlayList)
    self.userActivity = openPlaylist
    openPlaylist.becomeCurrent()
    PlaylistP3A.recordUsage()

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
              webLoaderFactory: LivePlaylistWebLoaderFactory(
                profile: self.profileController.profile
              )
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
      let alert = UIAlertController(
        title: Strings.PlayList.playlistDiskSpaceWarningTitle,
        message: Strings.PlayList.playlistDiskSpaceWarningMessage,
        preferredStyle: .alert
      )

      alert.addAction(
        UIAlertAction(
          title: Strings.PlayList.playlistDiskSpaceAddAnywayButtonTitle,
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
}

extension BrowserViewController {
  func openPlaylistSettingsMenu() {
    let playlistSettings = PlaylistSettingsViewController()
    let navigationController = UINavigationController(rootViewController: playlistSettings)
    self.present(navigationController, animated: true)
  }
}
