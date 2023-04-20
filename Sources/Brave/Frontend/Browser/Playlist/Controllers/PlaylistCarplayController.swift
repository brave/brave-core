// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Combine
import CarPlay
import MediaPlayer
import Data
import Preferences
import Shared
import CoreData
import Favicon
import os.log

private enum PlaylistCarPlayTemplateID: String {
  case folders
  case itemsList
  case settings
}

enum PlaylistCarplayError: Error {
  case cancelled
  case invalidItem(id: String?)
  case itemExpired(id: String)
}

class PlaylistCarplayController: NSObject {
  private let player: MediaPlayer
  private let mediaStreamer: PlaylistMediaStreamer
  private let interfaceController: CPInterfaceController
  private var playerStateObservers = Set<AnyCancellable>()
  private var assetStateObservers = Set<AnyCancellable>()
  private var assetLoadingStateObservers = Set<AnyCancellable>()
  private var playlistObservers = Set<AnyCancellable>()
  private let savedFolder = PlaylistFolder.getFolder(uuid: PlaylistFolder.savedFolderUUID)
  private let foldersFRC = PlaylistFolder.frc(savedFolder: false, sharedFolders: false)
  private let sharedFoldersFRC = PlaylistFolder.frc(savedFolder: false, sharedFolders: true)

  // For now, I have absolutely ZERO idea why the API says:
  // CPAllowedTemplates = CPAlertTemplate, invalid object CPActionSheetTemplate
  // So we support both, but force alerts to be safe for now.
  // MIGHT just be a simulator bug in the validation check.
  private static var mustUseCPAlertTemplate = true

  init(mediaStreamer: PlaylistMediaStreamer, player: MediaPlayer, interfaceController: CPInterfaceController) {
    self.player = player
    self.interfaceController = interfaceController
    self.mediaStreamer = mediaStreamer
    super.init()

    foldersFRC.delegate = self
    sharedFoldersFRC.delegate = self
    interfaceController.delegate = self

    observeFolderStates()
    observePlayerStates()
    observePlaylistStates()
    PlaylistManager.shared.reloadData()
    doLayout()

    DispatchQueue.main.async {
      // Workaround to see carplay NowPlaying on the simulator
      #if targetEnvironment(simulator)
      UIApplication.shared.endReceivingRemoteControlEvents()
      UIApplication.shared.beginReceivingRemoteControlEvents()
      #endif
    }
  }

  func observeFolderStates() {
    PlaylistCarplayManager.shared.onCarplayUIChangedToRoot.eraseToAnyPublisher()
      .receive(on: DispatchQueue.main)
      .sink { [weak self] in
        guard let self = self else { return }
        self.interfaceController.popToRootTemplate(animated: true) { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        }
      }.store(in: &playlistObservers)

    PlaylistManager.shared.onCurrentFolderDidChange
      .receive(on: DispatchQueue.main)
      .sink { [weak self] in
        guard let self = self else { return }

        if self.interfaceController.rootTemplate == self.interfaceController.topTemplate {
          // Nothing to Pop
          // Push the Folder Template
          let listTemplate = self.generatePlaylistListTemplate()
          self.interfaceController.pushTemplate(listTemplate, animated: true) { [weak self] success, error in
            if !success, let error = error {
              self?.displayErrorAlert(error: error)
            }
          }
          return
        }

        // Pop to Root Template
        self.interfaceController.popToRootTemplate(animated: false) { success, error in
          if !success, let error = error {
            self.displayErrorAlert(error: error)
          } else {
            // Push the Folder Template
            let listTemplate = self.generatePlaylistListTemplate()
            self.interfaceController.pushTemplate(listTemplate, animated: true) { [weak self] success, error in
              if !success, let error = error {
                self?.displayErrorAlert(error: error)
              }
            }
          }
        }
      }.store(in: &playlistObservers)
  }

  func observePlayerStates() {
    player.publisher(for: .play).sink { [weak self] event in
      guard let self = self else { return }

      MPNowPlayingInfoCenter.default().playbackState = .playing
      PlaylistMediaStreamer.updateNowPlayingInfo(event.mediaPlayer)

      // Update the playing item indicator & Cache State Icon
      guard let tabTemplate = self.interfaceController.rootTemplate as? CPTabBarTemplate else {
        return
      }

      // The folder is already showing all its items
      var currentTemplate = self.interfaceController.topTemplate as? CPListTemplate

      // If the folder's items isn't showing, then we're either on the folder view or settings view
      // Therefore we need to push the folder view onto the stack
      if currentTemplate == nil || (currentTemplate?.userInfo as? [String: String])?["id"] != PlaylistCarPlayTemplateID.itemsList.rawValue {

        // Fetch the root playlistTabTemplate
        currentTemplate = tabTemplate.templates.compactMap({ $0 as? CPListTemplate }).first(where: {
          ($0.userInfo as? [String: String])?["id"] == PlaylistCarPlayTemplateID.itemsList.rawValue
        })
      }

      // We need to ensure the template that is showing is the list of playlist items
      guard (currentTemplate?.userInfo as? [String: String])?["id"] == PlaylistCarPlayTemplateID.itemsList.rawValue else {
        return
      }

      if let playlistTabTemplate = currentTemplate {
        let items = playlistTabTemplate.sections.flatMap({ $0.items }).compactMap({ $0 as? CPListItem })

        items.forEach({
          if let userInfo = $0.userInfo as? [String: Any],
            let itemId = userInfo["id"] as? String {

            $0.accessoryType = PlaylistManager.shared.state(for: itemId) != .downloaded ? .cloud : .none

            if PlaylistCarplayManager.shared.currentPlaylistItem?.tagId == itemId {
              $0.isPlaying = true

              PlaylistMediaStreamer.setNowPlayingMediaArtwork(image: userInfo["icon"] as? UIImage)
            } else {
              $0.isPlaying = false
            }
          }
        })
      }

      tabTemplate.updateTemplates(tabTemplate.templates)

      if self.interfaceController.topTemplate != CPNowPlayingTemplate.shared {
        self.interfaceController.pushTemplate(CPNowPlayingTemplate.shared, animated: true) { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        }
      }
    }.store(in: &playerStateObservers)

    player.publisher(for: .pause).sink { event in
      MPNowPlayingInfoCenter.default().playbackState = .paused
      PlaylistMediaStreamer.updateNowPlayingInfo(event.mediaPlayer)
    }.store(in: &playerStateObservers)

    player.publisher(for: .stop).sink { [weak self] _ in
      guard let self = self else { return }
      MPNowPlayingInfoCenter.default().playbackState = .stopped

      if self.interfaceController.topTemplate == CPNowPlayingTemplate.shared {
        self.interfaceController.popTemplate(animated: false) { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        }
      }
    }.store(in: &playerStateObservers)

    player.publisher(for: .changePlaybackRate).sink { event in
      MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackRate] = event.mediaPlayer.rate
    }.store(in: &playerStateObservers)

    player.publisher(for: .previousTrack).sink { [weak self] _ in
      self?.onPreviousTrack(isUserInitiated: true)
    }.store(in: &playerStateObservers)

    player.publisher(for: .nextTrack).sink { [weak self] _ in
      self?.onNextTrack(isUserInitiated: true)
    }.store(in: &playerStateObservers)

    player.publisher(for: .finishedPlaying).sink { [weak self] event in
      guard let self = self else { return }
      
      event.mediaPlayer.pause()
      event.mediaPlayer.seek(to: .zero)
      
      if let item = PlaylistCarplayManager.shared.currentPlaylistItem {
        self.updateLastPlayedItem(item: item)
      }

      var nowPlayingInfo = MPNowPlayingInfoCenter.default().nowPlayingInfo
      nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackProgress] = 0.0
      nowPlayingInfo?[MPNowPlayingInfoPropertyElapsedPlaybackTime] = 0.0
      nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackRate] = 0.0
      MPNowPlayingInfoCenter.default().nowPlayingInfo = nowPlayingInfo

      self.onNextTrack(isUserInitiated: false)
    }.store(in: &playerStateObservers)
  }

  func observePlaylistStates() {
    let reloadData = { [weak self] in
      self?.reloadData()
    }

    PlaylistManager.shared.contentDidChange
      .sink { _ in
        reloadData()
      }.store(in: &playlistObservers)

    PlaylistManager.shared.downloadStateChanged
      .sink { _ in
        reloadData()
      }.store(in: &playlistObservers)
  }

  private func doLayout() {
    do {
      try foldersFRC.performFetch()
      try sharedFoldersFRC.performFetch()
    } catch {
      Logger.module.error("\(error.localizedDescription)")
      displayErrorAlert(error: error)
    }

    // FOLDERS TEMPLATE
    let foldersTemplate = generatePlaylistFolderListTemplate()

    // SETTINGS TEMPLATE
    let settingsTemplate = generateSettingsTemplate()

    // ALL TEMPLATES
    let tabTemplates: [CPTemplate] = [
      foldersTemplate,
      settingsTemplate,
    ]

    self.interfaceController.delegate = self
    self.interfaceController.setRootTemplate(
      CPTabBarTemplate(templates: tabTemplates),
      animated: true,
      completion: { [weak self] success, error in
        if !success, let error = error {
          self?.displayErrorAlert(error: error)
        }
      })
  }

  private func reloadData() {
    guard let tabBarTemplate = interfaceController.rootTemplate as? CPTabBarTemplate else {
      return
    }

    // Need to unwind the navigation stack to the root before attempting any modifications
    // Some cars will crash if we replace or modify the RootTemplate while
    // an alert of CPNowPlayingTemplate.shared is being displayed
    // interfaceController.pop(to: tabBarTemplate, animated: true)

    // Update Currently Playing Index before layout (so we can show the playing indicator)
    if let itemId = PlaylistCarplayManager.shared.currentPlaylistItem?.tagId {
      PlaylistCarplayManager.shared.currentlyPlayingItemIndex = PlaylistManager.shared.index(of: itemId) ?? -1
    }

    do {
      try foldersFRC.performFetch()
      try sharedFoldersFRC.performFetch()
    } catch {
      Logger.module.error("\(error.localizedDescription)")
    }

    // FOLDERS TEMPLATE
    let foldersTemplate = generatePlaylistFolderListTemplate()

    // SETTINGS TEMPLATE
    let settingsTemplate = generateSettingsTemplate()

    // ALL TEMPLATES
    let tabTemplates: [CPTemplate] = [
      foldersTemplate,
      settingsTemplate,
    ]

    // If we have any controllers presented, we need to remove them.
    interfaceController.popToRootTemplate(animated: true) { success, error in
      if !success, let error = error {
        Logger.module.error("\(error.localizedDescription)")
      }
    }

    // Reload the templates instead of replacing the RootTemplate
    tabBarTemplate.updateTemplates(tabTemplates)
  }

  private func generatePlaylistFolderListTemplate() -> CPTemplate {
    // Fetch all Playlist Folders
    let folders = foldersFRC.fetchedObjects ?? []
    let sharedFolders = sharedFoldersFRC.fetchedObjects ?? []

    // Construct Folders UI
    let itemCount = savedFolder?.playlistItems?.count ?? 0
    let savedFolder = CPListItem(
      text: savedFolder?.title ?? Strings.PlaylistFolders.playlistUntitledFolderTitle,
      detailText: "\(itemCount == 1 ? Strings.PlaylistFolders.playlistFolderSubtitleItemSingleCount : String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderSubtitleItemCount, itemCount))",
      image: nil,
      accessoryImage: nil,
      accessoryType: .disclosureIndicator
    ).then {
      $0.handler = { [unowned self] _, completion in
        // Display items in this folder
        PlaylistManager.shared.currentFolder = self.savedFolder
        completion()
      }

      $0.userInfo = ["uuid": self.savedFolder?.uuid]
    }

    let folderItems = folders.compactMap { folder -> CPListItem? in
      let itemCount = folder.playlistItems?.count ?? 0
      return CPListItem(
        text: folder.title ?? Strings.PlaylistFolders.playlistUntitledFolderTitle,
        detailText: "\(itemCount == 1 ? Strings.PlaylistFolders.playlistFolderSubtitleItemSingleCount : String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderSubtitleItemCount, itemCount))",
        image: nil,
        accessoryImage: nil,
        accessoryType: .disclosureIndicator
      ).then {
        $0.handler = { _, completion in
          // Display items in this folder
          PlaylistManager.shared.currentFolder = folder
          completion()
        }

        $0.userInfo = ["uuid": folder.uuid]
      }
    }
    
    let sharedFolderItems = sharedFolders.compactMap { folder -> CPListItem? in
      let itemCount = folder.playlistItems?.count ?? 0
      return CPListItem(
        text: folder.title ?? Strings.PlaylistFolders.playlistUntitledFolderTitle,
        detailText: "\(itemCount == 1 ? Strings.PlaylistFolders.playlistFolderSubtitleItemSingleCount : String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderSubtitleItemCount, itemCount))",
        image: nil,
        accessoryImage: nil,
        accessoryType: .disclosureIndicator
      ).then {
        $0.handler = { _, completion in
          // Display items in this folder
          PlaylistManager.shared.currentFolder = folder
          completion()
        }

        $0.userInfo = ["uuid": folder.uuid]
      }
    }

    // Template
    let foldersTemplate = CPListTemplate(
      title: Strings.PlayList.playlistCarplayTitle,
      sections: [
        CPListSection(items: [savedFolder]),
        CPListSection(items: folderItems),
        CPListSection(items: sharedFolderItems)
      ]
    ).then {
      $0.tabImage = UIImage(systemName: "list.star")
      $0.emptyViewTitleVariants = [Strings.PlayList.noItemLabelTitle]
      $0.emptyViewSubtitleVariants = [Strings.PlayList.noItemLabelDetailLabel]
      $0.userInfo = ["id": PlaylistCarPlayTemplateID.folders.rawValue]
    }
    return foldersTemplate
  }

  private func generatePlaylistListTemplate() -> CPTemplate {
    // Map all items to their IDs
    let playlistItemIds = PlaylistManager.shared.allItems.map { $0.tagId }

    // Fetch all Playlist Items
    var listItems = [CPListItem]()
    listItems = playlistItemIds.compactMap { itemId -> CPListItem? in
      guard let itemIndex = PlaylistManager.shared.index(of: itemId),
        let item = PlaylistManager.shared.itemAtIndex(itemIndex)
      else {
        return nil
      }

      let listItem = CPListItem(text: item.name, detailText: item.pageSrc)
      listItem.handler = { [unowned self, weak listItem] selectableItem, completion in
        let listItem = selectableItem as? CPListItem ?? listItem
        listItem?.accessoryType = .none

        self.initiatePlaybackOfItem(itemId: itemId) { error in
          guard let listItem = listItem else {
            completion()
            return
          }

          if let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }

          listItem.accessoryType = PlaylistManager.shared.state(for: itemId) != .downloaded ? .cloud : .none

          let isPlaying = self.player.isPlaying || self.player.isWaitingToPlay
          for item in listItems.enumerated() {
            let userInfo = item.element.userInfo as? [String: Any] ?? [:]
            item.element.isPlaying = isPlaying && (PlaylistCarplayManager.shared.currentPlaylistItem?.src == userInfo["id"] as? String)
          }

          let userInfo = listItem.userInfo as? [String: Any]
          PlaylistMediaStreamer.setNowPlayingMediaArtwork(image: userInfo?["icon"] as? UIImage)
          PlaylistMediaStreamer.updateNowPlayingInfo(self.player)

          completion()

          if error == nil && self.interfaceController.topTemplate != CPNowPlayingTemplate.shared {
            self.interfaceController.pushTemplate(CPNowPlayingTemplate.shared, animated: true) { success, error in

              if !success, let error = error {
                Logger.module.error("\(error.localizedDescription)")
              }
            }
          }
        }
      }

      // Update the current playing status
      listItem.isPlaying = player.isPlaying && (PlaylistCarplayManager.shared.currentPlaylistItem?.src == item.src)

      listItem.accessoryType = PlaylistManager.shared.state(for: itemId) != .downloaded ? .cloud : .none
      listItem.setImage(Favicon.defaultImage)
      var userInfo = [String: Any]()
      userInfo.merge(with: [
        "id": itemId,
        "thumbnailRenderer": self.loadThumbnail(for: item) {
          [weak listItem]
          image in
          guard let listItem = listItem else { return }

          // Store the thumbnail that was fetched
          userInfo["icon"] = image
          listItem.userInfo = userInfo

          if let image = image {
            listItem.setImage(image.scale(toSize: CPListItem.maximumImageSize))
          }

          if listItem.isPlaying {
            PlaylistMediaStreamer.setNowPlayingMediaArtwork(image: image)
          }

          // After completion, remove the renderer from the user info
          // so that it can dealloc nicely and save us some memory.
          DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            userInfo["thumbnailRenderer"] = nil
            listItem.userInfo = userInfo
          }
        } as Any,
      ])
      listItem.userInfo = userInfo
      return listItem
    }

    // Template
    let playlistTemplate = CPListTemplate(
      title: PlaylistManager.shared.currentFolder?.title ?? "",
      sections: [CPListSection(items: listItems)]
    ).then {
      $0.tabImage = UIImage(systemName: "list.star")
      $0.emptyViewTitleVariants = [Strings.PlayList.noItemLabelTitle]
      $0.emptyViewSubtitleVariants = [Strings.PlayList.noItemLabelDetailLabel]
      $0.userInfo = ["id": PlaylistCarPlayTemplateID.itemsList.rawValue]
    }
    return playlistTemplate
  }

  private func generateSettingsTemplate() -> CPTemplate {
    // Playback Options
    let restartPlaybackOption = CPListItem(
      text: Strings.PlayList.playlistCarplayRestartPlaybackOptionTitle,
      detailText: Strings.PlayList.playlistCarplayRestartPlaybackOptionDetailsTitle
    ).then {
      $0.handler = { [unowned self] listItem, completion in
        let enableGridView = Preferences.Playlist.enableCarPlayRestartPlayback.value
        let title = enableGridView ? Strings.PlayList.playlistCarplayRestartPlaybackButtonStateEnabled : Strings.PlayList.playlistCarplayRestartPlaybackButtonStateDisabled
        let icon = enableGridView ? UIImage(named: "checkbox_on", in: .module, compatibleWith: nil)! : UIImage(named: "loginUnselected", in: .module, compatibleWith: nil)!
        let button = CPGridButton(titleVariants: [title], image: icon) { [unowned self] button in
          Preferences.Playlist.enableCarPlayRestartPlayback.value = !enableGridView
          self.interfaceController.popTemplate(
            animated: true,
            completion: { success, error in
              if !success, let error = error {
                Logger.module.error("\(error.localizedDescription)")
              }
            })
        }

        self.interfaceController.pushTemplate(
          CPGridTemplate(title: Strings.PlayList.playlistCarplayOptionsScreenTitle, gridButtons: [button]),
          animated: true,
          completion: { success, error in
            if !success, let error = error {
              Logger.module.error("\(error.localizedDescription)")
            }
          }
        )

        completion()
      }
    }

    // Sections
    let playbackSection = CPListSection(
      items: [restartPlaybackOption],
      header: Strings.PlayList.playlistCarplayOptionsScreenTitle,
      sectionIndexTitle: Strings.PlayList.playlistCarplayOptionsScreenTitle)

    // Template
    let settingsTemplate = CPListTemplate(
      title: Strings.PlayList.playlistCarplaySettingsSectionTitle,
      sections: [playbackSection]
    ).then {
      $0.tabImage = UIImage(systemName: "gear")
      $0.userInfo = ["id": PlaylistCarPlayTemplateID.settings.rawValue]
    }
    return settingsTemplate
  }
}

extension PlaylistCarplayController: CPInterfaceControllerDelegate {
  func templateWillAppear(_ aTemplate: CPTemplate, animated: Bool) {
    Logger.module.debug("Template \(aTemplate.classForCoder) will appear.")

    if interfaceController.topTemplate != CPNowPlayingTemplate.shared,
      (aTemplate.userInfo as? [String: String])?["id"] == PlaylistCarPlayTemplateID.folders.rawValue {
      self.stop()

      PlaylistCarplayManager.shared.onCarplayUIChangedToRoot.send()
    }
  }

  func templateDidAppear(_ aTemplate: CPTemplate, animated: Bool) {
    Logger.module.debug("Template \(aTemplate.classForCoder) did appear.")
  }

  func templateWillDisappear(_ aTemplate: CPTemplate, animated: Bool) {
    Logger.module.debug("Template \(aTemplate.classForCoder) will disappear.")
  }

  func templateDidDisappear(_ aTemplate: CPTemplate, animated: Bool) {
    Logger.module.debug("Template \(aTemplate.classForCoder) did disappear.")
  }
}

extension PlaylistCarplayController: NSFetchedResultsControllerDelegate {
  func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {

    reloadData()
  }

  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    reloadData()
  }
}

extension PlaylistCarplayController {

  func loadThumbnail(for mediaItem: PlaylistInfo, completion: @escaping (UIImage?) -> Void) -> PlaylistThumbnailRenderer? {
    guard let assetUrl = URL(string: mediaItem.src),
      let favIconUrl = URL(string: mediaItem.pageSrc)
    else {
      completion(nil)
      return nil
    }

    let thumbnailRenderer = PlaylistThumbnailRenderer()
    thumbnailRenderer.loadThumbnail(
      assetUrl: assetUrl,
      favIconUrl: favIconUrl,
      completion: { image in
        completion(image)
      })
    return thumbnailRenderer
  }

  func initiatePlaybackOfItem(itemId: String, completionHandler: @escaping (Error?) -> Void) {
    guard let index = PlaylistManager.shared.index(of: itemId),
      let item = PlaylistManager.shared.itemAtIndex(index)
    else {
      displayLoadingResourceError()
      completionHandler(PlaylistCarplayError.invalidItem(id: itemId))
      return
    }

    if !Preferences.Playlist.enableCarPlayRestartPlayback.value {
      // Item is already playing.
      // Show now-playing screen and don't restart playback.
      if PlaylistCarplayManager.shared.currentPlaylistItem?.tagId == itemId {
        // If the player is currently paused, un-pause it and play the item.
        // If the player is currently stopped, do nothing.
        if !player.isPlaying, player.currentItem != nil {
          play()
        }
        completionHandler(nil)
        return
      }
    }

    // Reset Now Playing when playback is starting.
    PlaylistMediaStreamer.clearNowPlayingInfo()

    PlaylistCarplayManager.shared.currentPlaylistItem = item
    PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
    self.playItem(item: item) { [weak self] error in
      guard let self = self else {
        PlaylistCarplayManager.shared.currentPlaylistItem = nil
        PlaylistCarplayManager.shared.currentlyPlayingItemIndex = -1
        completionHandler(nil)
        return
      }

      if !self.player.isPlaying {
        PlaylistCarplayManager.shared.currentPlaylistItem = nil
        PlaylistCarplayManager.shared.currentlyPlayingItemIndex = -1
      }

      switch error {
      case .other(let error):
        Logger.module.error("\(error.localizedDescription)")
        self.displayLoadingResourceError()
        completionHandler(error)
      case .cannotLoadMedia:
        self.displayLoadingResourceError()
        completionHandler(PlaylistCarplayError.cancelled)
      case .expired:
        self.displayExpiredResourceError(item: item)
        completionHandler(PlaylistCarplayError.itemExpired(id: itemId))
      case .none:
        PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
        PlaylistCarplayManager.shared.currentPlaylistItem = item
        completionHandler(nil)
      case .cancelled:
        Logger.module.debug("User Cancelled Playlist playback")
        completionHandler(PlaylistCarplayError.cancelled)
      }

      // Workaround to see carplay NowPlaying on the simulator
      #if targetEnvironment(simulator)
      DispatchQueue.main.async {
        UIApplication.shared.endReceivingRemoteControlEvents()
        UIApplication.shared.beginReceivingRemoteControlEvents()
      }
      #endif
    }
  }
}

extension PlaylistCarplayController {
  func onPreviousTrack(isUserInitiated: Bool) {
    if PlaylistCarplayManager.shared.currentlyPlayingItemIndex <= 0 {
      return
    }

    let index = PlaylistCarplayManager.shared.currentlyPlayingItemIndex - 1
    if index < PlaylistManager.shared.numberOfAssets,
      let item = PlaylistManager.shared.itemAtIndex(index) {
      PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
      playItem(item: item) { [weak self] error in
        PlaylistCarplayManager.shared.currentPlaylistItem = nil
        guard let self = self else { return }

        switch error {
        case .other(let err):
          Logger.module.error("\(err.localizedDescription)")
          fallthrough
        case .cannotLoadMedia:
          self.displayLoadingResourceError()
        case .expired:
          self.displayExpiredResourceError(item: item)
        case .none:
          PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
          PlaylistCarplayManager.shared.currentPlaylistItem = item
          self.updateLastPlayedItem(item: item)
        case .cancelled:
          Logger.module.debug("User Cancelled Playlist Playback")
        }
      }
    }
  }

  func onNextTrack(isUserInitiated: Bool) {
    let assetCount = PlaylistManager.shared.numberOfAssets
    let isAtEnd = PlaylistCarplayManager.shared.currentlyPlayingItemIndex >= assetCount - 1
    var index = PlaylistCarplayManager.shared.currentlyPlayingItemIndex

    switch player.repeatState {
    case .none:
      if isAtEnd {
        player.pictureInPictureController?.delegate = nil
        player.pictureInPictureController?.stopPictureInPicture()
        player.pause()

        PlaylistCarplayManager.shared.playlistController = nil
        return
      }
      index += 1
    case .repeatOne:
      player.seek(to: 0.0)
      player.play()
      return
    case .repeatAll:
      index = isAtEnd ? 0 : index + 1
    }

    if index >= 0,
      let item = PlaylistManager.shared.itemAtIndex(index) {
      PlaylistCarplayManager.shared.currentPlaylistItem = item
      PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
      self.playItem(item: item) { [weak self] error in
        PlaylistCarplayManager.shared.currentPlaylistItem = nil
        guard let self = self else { return }

        switch error {
        case .other(let err):
          Logger.module.error("\(err.localizedDescription)")
          fallthrough
        case .cannotLoadMedia:
          self.displayLoadingResourceError()
        case .expired:
          if isUserInitiated || self.player.repeatState == .repeatOne || assetCount <= 1 {
            self.displayExpiredResourceError(item: item)
          } else {
            DispatchQueue.main.async {
              PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
              self.onNextTrack(isUserInitiated: isUserInitiated)
            }
          }
        case .none:
          PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
          PlaylistCarplayManager.shared.currentPlaylistItem = item
          self.updateLastPlayedItem(item: item)
        case .cancelled:
          Logger.module.debug("User Cancelled Playlist Playback")
        }
      }
    }
  }

  private func play() {
    player.play()
  }

  private func pause() {
    player.pause()
  }

  private func stop() {
    PlaylistMediaStreamer.clearNowPlayingInfo()

    PlaylistCarplayManager.shared.currentlyPlayingItemIndex = -1
    PlaylistCarplayManager.shared.currentPlaylistItem = nil
    player.stop()

    // Cancel all loading.
    assetLoadingStateObservers.removeAll()
    assetStateObservers.removeAll()
  }

  private func seekBackwards() {
    player.seekBackwards()
  }

  private func seekForwards() {
    player.seekForwards()
  }

  private func seek(to time: TimeInterval) {
    player.seek(to: time)
  }

  func seek(relativeOffset: Float) {
    if let currentItem = player.currentItem {
      let seekTime = CMTimeMakeWithSeconds(Float64(CGFloat(relativeOffset) * CGFloat(currentItem.asset.duration.value) / CGFloat(currentItem.asset.duration.timescale)), preferredTimescale: currentItem.currentTime().timescale)
      seek(to: seekTime.seconds)
    }
  }

  func load(url: URL, autoPlayEnabled: Bool) -> AnyPublisher<Void, Error> {
    load(asset: AVURLAsset(url: url, options: AVAsset.defaultOptions), autoPlayEnabled: autoPlayEnabled)
  }

  func load(asset: AVURLAsset, autoPlayEnabled: Bool) -> AnyPublisher<Void, Error> {
    assetLoadingStateObservers.removeAll()
    player.stop()

    return Future { [weak self] resolver in
      guard let self = self else {
        resolver(.failure(PlaylistCarplayError.cancelled))
        return
      }

      self.player.load(asset: asset)
        .receive(on: RunLoop.main)
        .sink(
          receiveCompletion: { status in
            switch status {
            case .failure(let error):
              resolver(.failure(error))
            case .finished:
              break
            }
          },
          receiveValue: { [weak self] isNewItem in
            guard let self = self else {
              resolver(.failure(PlaylistCarplayError.cancelled))
              return
            }

            guard self.player.currentItem != nil else {
              resolver(.failure(PlaylistCarplayError.invalidItem(id: nil)))
              return
            }

            // We are playing the same item again..
            if !isNewItem {
              self.pause()
              self.seek(relativeOffset: 0.0)  // Restart playback
              self.play()
              resolver(.success(Void()))
              return
            }

            // Track-bar
            if autoPlayEnabled {
              self.play()  // Play the new item
              resolver(.success(Void()))
            }
          }
        ).store(in: &self.assetLoadingStateObservers)
    }.eraseToAnyPublisher()
  }

  func playItem(item: PlaylistInfo, completion: ((PlaylistMediaStreamer.PlaybackError) -> Void)?) {
    let isPlaying = player.isPlaying
    assetLoadingStateObservers.removeAll()
    assetStateObservers.removeAll()

    // If the item is cached, load it from the cache and play it.
    let cacheState = PlaylistManager.shared.state(for: item.tagId)
    if cacheState != .invalid {
      if let index = PlaylistManager.shared.index(of: item.tagId),
        let asset = PlaylistManager.shared.assetAtIndex(index) {
        load(asset: asset, autoPlayEnabled: true)
          .handleEvents(receiveCancel: {
            if !isPlaying {
              PlaylistMediaStreamer.clearNowPlayingInfo()
            }
            completion?(.cancelled)
          })
          .sink(
            receiveCompletion: { status in
              switch status {
              case .failure(let error):
                if !isPlaying {
                  PlaylistMediaStreamer.clearNowPlayingInfo()
                }
                completion?(.other(error))
              case .finished:
                break
              }
            },
            receiveValue: { [weak self] _ in
              guard let self = self else {
                if !isPlaying {
                  PlaylistMediaStreamer.clearNowPlayingInfo()
                }
                completion?(.cancelled)
                return
              }

              PlaylistMediaStreamer.setNowPlayingInfo(item, withPlayer: self.player)
              completion?(.none)
            }
          ).store(in: &assetLoadingStateObservers)
      } else {
        completion?(.expired)
      }
      return
    }

    // The item is not cached so we should attempt to stream it
    streamItem(item: item, completion: completion)
  }

  func streamItem(item: PlaylistInfo, completion: ((PlaylistMediaStreamer.PlaybackError) -> Void)?) {
    let isPlaying = player.isPlaying
    assetStateObservers.removeAll()

    mediaStreamer.loadMediaStreamingAsset(item)
      .handleEvents(receiveCancel: {
        if !isPlaying {
          PlaylistMediaStreamer.clearNowPlayingInfo()
        }
        completion?(.cancelled)
      })
      .sink(
        receiveCompletion: { status in
          switch status {
          case .failure(let error):
            if !isPlaying {
              PlaylistMediaStreamer.clearNowPlayingInfo()
            }
            completion?(error)
          case .finished:
            break
          }
        },
        receiveValue: { [weak self] _ in
          guard let self = self else {
            if !isPlaying {
              PlaylistMediaStreamer.clearNowPlayingInfo()
            }
            completion?(.cancelled)
            return
          }

          // Item can be streamed, so let's retrieve its URL from our DB
          guard let index = PlaylistManager.shared.index(of: item.tagId),
            let item = PlaylistManager.shared.itemAtIndex(index)
          else {
            if !isPlaying {
              PlaylistMediaStreamer.clearNowPlayingInfo()
            }
            completion?(.expired)
            return
          }

          // Attempt to play the stream
          if let url = URL(string: item.src) {
            self.load(url: url, autoPlayEnabled: true)
              .handleEvents(receiveCancel: {
                if !isPlaying {
                  PlaylistMediaStreamer.clearNowPlayingInfo()
                }
                completion?(.cancelled)
              })
              .sink(
                receiveCompletion: { status in
                  switch status {
                  case .failure(let error):
                    if !isPlaying {
                      PlaylistMediaStreamer.clearNowPlayingInfo()
                    }

                    switch error as? MediaPlaybackError {
                    case .cancelled:
                      if !isPlaying {
                        PlaylistMediaStreamer.clearNowPlayingInfo()
                      }
                      completion?(.cancelled)
                    case .other(let err):
                      completion?(.other(err))
                    default:
                      completion?(.other(error))
                    }
                  case .finished:
                    break
                  }
                },
                receiveValue: { [weak self] _ in
                  guard let self = self else {
                    if !isPlaying {
                      PlaylistMediaStreamer.clearNowPlayingInfo()
                    }
                    completion?(.cancelled)
                    return
                  }

                  PlaylistMediaStreamer.setNowPlayingInfo(item, withPlayer: self.player)
                  completion?(.none)
                }
              ).store(in: &self.assetLoadingStateObservers)
            Logger.module.debug("Playing Live Video: \(self.player.isLiveMedia)")
          } else {
            if !isPlaying {
              PlaylistMediaStreamer.clearNowPlayingInfo()
            }
            completion?(.expired)
          }
        }
      ).store(in: &assetStateObservers)
  }

  func updateLastPlayedItem(item: PlaylistInfo) {
    guard let playTime = player.currentItem?.currentTime() else {
      return
    }
    
    let lastPlayedTime = Preferences.Playlist.playbackLeftOff.value ? playTime.seconds : 0.0
    PlaylistItem.updateLastPlayed(itemId: item.tagId, pageSrc: item.pageSrc, lastPlayedOffset: lastPlayedTime)
  }

  func displayExpiredResourceError(item: PlaylistInfo?) {
    // CarPlay cannot "open" URLs so we display an alert with an okay button only.
    // Maybe in the future, check if the phone is open, if it is, display the alert there.
    // and the user can "open" the item in the webView/browser.

    if PlaylistCarplayController.mustUseCPAlertTemplate {
      // Some cars do NOT support CPActionSheetTemplate,
      // So we MUST use CPAlertTemplate
      let alert = CPAlertTemplate(
        titleVariants: [Strings.PlayList.expiredAlertTitle],
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle, style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                })
            })
        ])

      interfaceController.presentTemplate(
        alert, animated: true,
        completion: { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        })
    } else {
      let alert = CPActionSheetTemplate(
        title: Strings.PlayList.expiredAlertTitle,
        message: Strings.PlayList.expiredAlertDescription,
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle, style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                })
            })
        ])

      interfaceController.presentTemplate(
        alert, animated: true,
        completion: { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        })
    }
  }

  func displayLoadingResourceError() {
    if PlaylistCarplayController.mustUseCPAlertTemplate {
      // Some cars do NOT support CPActionSheetTemplate,
      // So we MUST use CPAlertTemplate
      let alert = CPAlertTemplate(
        titleVariants: [Strings.PlayList.loadResourcesErrorAlertDescription],
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle, style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                })
            })
        ])

      interfaceController.presentTemplate(
        alert, animated: true,
        completion: { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        })
    } else {
      // Can also use CPAlertTemplate, but it doesn't have a "Message" parameter.
      let alert = CPActionSheetTemplate(
        title: Strings.PlayList.sorryAlertTitle,
        message: Strings.PlayList.loadResourcesErrorAlertDescription,
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle, style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                })
            })
        ])

      interfaceController.presentTemplate(
        alert, animated: true,
        completion: { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        })
    }
  }

  private func displayErrorAlert(error: Error) {
    // Some cars do NOT support CPActionSheetTemplate
    // So we MUST use CPAlertTemplate
    if PlaylistCarplayController.mustUseCPAlertTemplate {
      let alert = CPAlertTemplate(
        titleVariants: [error.localizedDescription],
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle, style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                })
            })
        ])

      DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
        self.interfaceController.presentTemplate(
          alert, animated: true,
          completion: { success, error in
            if !success, let error = error {
              Logger.module.error("\(error.localizedDescription)")
            }
          })
      }
    } else {
      // Can also use CPAlertTemplate, but it doesn't have a "Message" parameter.
      let alert = CPActionSheetTemplate(
        title: Strings.PlayList.sorryAlertTitle,
        message: error.localizedDescription,
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle, style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                })
            })
        ])

      DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
        self.interfaceController.presentTemplate(
          alert, animated: true,
          completion: { success, error in
            if !success, let error = error {
              Logger.module.error("\(error.localizedDescription)")
            }
          })
      }
    }
  }
}
