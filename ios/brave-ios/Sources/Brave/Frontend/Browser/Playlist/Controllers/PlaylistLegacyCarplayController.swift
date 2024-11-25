// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CarPlay
import Combine
import CoreData
import Data
import Favicon
import Foundation
import MediaPlayer
import Playlist
import Preferences
import Shared
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

class PlaylistLegacyCarplayController: NSObject {
  private let player: MediaPlayer
  private let mediaStreamer: PlaylistMediaStreamer
  private let interfaceController: CPInterfaceController
  private var playerStateObservers = Set<AnyCancellable>()
  private var playlistObservers = Set<AnyCancellable>()
  private let savedFolder = PlaylistFolder.getFolder(uuid: PlaylistFolder.savedFolderUUID)
  private let foldersFRC = PlaylistFolder.frc(savedFolder: false, sharedFolders: false)
  private let sharedFoldersFRC = PlaylistFolder.frc(savedFolder: false, sharedFolders: true)

  // For now, I have absolutely ZERO idea why the API says:
  // CPAllowedTemplates = CPAlertTemplate, invalid object CPActionSheetTemplate
  // So we support both, but force alerts to be safe for now.
  // MIGHT just be a simulator bug in the validation check.
  private static var mustUseCPAlertTemplate = true

  init(
    mediaStreamer: PlaylistMediaStreamer,
    player: MediaPlayer,
    interfaceController: CPInterfaceController
  ) {
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
    PlaylistManager.shared.onCurrentFolderDidChange
      .receive(on: DispatchQueue.main)
      .sink { [weak self] in
        guard let self = self else { return }

        if self.interfaceController.rootTemplate == self.interfaceController.topTemplate {
          // Nothing to Pop
          // Push the Folder Template
          let listTemplate = self.generatePlaylistListTemplate()
          self.interfaceController.pushTemplate(listTemplate, animated: true) {
            [weak self] success, error in
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
            self.interfaceController.pushTemplate(listTemplate, animated: true) {
              [weak self] success, error in
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
      NowPlayingInfo.updateNowPlayingInfo(event.mediaPlayer)

      // Update the playing item indicator & Cache State Icon
      guard let tabTemplate = self.interfaceController.rootTemplate as? CPTabBarTemplate else {
        return
      }

      if let playlistTabTemplate = self.getItemListTemplate() {
        let items = playlistTabTemplate.sections.flatMap({ $0.items }).compactMap({
          $0 as? CPListItem
        })

        items.forEach({
          if let userInfo = $0.userInfo as? [String: Any],
            let itemId = userInfo["id"] as? String
          {

            $0.accessoryType =
              PlaylistManager.shared.state(for: itemId) != .downloaded ? .cloud : .none

            if PlaylistCoordinator.shared.currentPlaylistItem?.tagId == itemId {
              $0.isPlaying = true

              NowPlayingInfo.setNowPlayingMediaArtwork(image: userInfo["icon"] as? UIImage)
            } else {
              $0.isPlaying = false
            }
          }
        })
      }

      tabTemplate.updateTemplates(tabTemplate.templates)

      if self.interfaceController.topTemplate != CPNowPlayingTemplate.shared {
        self.interfaceController.pushTemplate(CPNowPlayingTemplate.shared, animated: true) {
          success,
          error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        }
      }
    }.store(in: &playerStateObservers)

    player.publisher(for: .pause).sink { event in
      MPNowPlayingInfoCenter.default().playbackState = .paused
      NowPlayingInfo.updateNowPlayingInfo(event.mediaPlayer)
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
      MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackRate] =
        event.mediaPlayer.rate
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

      if let item = PlaylistCoordinator.shared.currentPlaylistItem {
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

  private func getFolderTemplateList() -> CPListTemplate? {
    // Retrieve the tab-bar
    guard let tabTemplate = self.interfaceController.rootTemplate as? CPTabBarTemplate else {
      return nil
    }

    // Retrieve the folder list item
    let folderTemplate = tabTemplate.templates.compactMap({ $0 as? CPListTemplate }).first(where: {
      ($0.userInfo as? [String: String])?["id"] == PlaylistCarPlayTemplateID.folders.rawValue
    })

    return folderTemplate
  }

  private func getItemListTemplate() -> CPListTemplate? {
    // Retrieve the item-list
    let listTemplate = interfaceController.templates.compactMap({ $0 as? CPListTemplate }).first(
      where: {
        ($0.userInfo as? [String: String])?["id"] == PlaylistCarPlayTemplateID.itemsList.rawValue
      })

    return listTemplate
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
      }
    )
  }

  private func reloadData() {
    // Update Currently Playing Index before layout (so we can show the playing indicator)
    if let itemId = PlaylistCoordinator.shared.currentPlaylistItem?.tagId {
      PlaylistCoordinator.shared.currentlyPlayingItemIndex =
        PlaylistManager.shared.index(of: itemId) ?? -1
    }

    // FOLDERS TEMPLATE
    guard let foldersTemplate = generatePlaylistFolderListTemplate() as? CPListTemplate else {
      Logger.module.error("Invalid Folders Template - NOT A CPListTemplate!")
      return
    }

    let oldFoldersTemplate = getFolderTemplateList()
    oldFoldersTemplate?.updateSections(foldersTemplate.sections)
  }

  private func generatePlaylistFolderListTemplate() -> CPTemplate {
    // Fetch all Playlist Folders
    let folders = foldersFRC.fetchedObjects ?? []
    let sharedFolders = sharedFoldersFRC.fetchedObjects ?? []

    // Construct Folders UI
    let itemCount = savedFolder?.playlistItems?.count ?? 0
    let savedFolder = CPListItem(
      text: savedFolder?.title ?? Strings.PlaylistFolders.playlistUntitledFolderTitle,
      detailText:
        "\(itemCount == 1 ? Strings.PlaylistFolders.playlistFolderSubtitleItemSingleCount : String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderSubtitleItemCount, itemCount))",
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
        detailText:
          "\(itemCount == 1 ? Strings.PlaylistFolders.playlistFolderSubtitleItemSingleCount : String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderSubtitleItemCount, itemCount))",
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
        detailText:
          "\(itemCount == 1 ? Strings.PlaylistFolders.playlistFolderSubtitleItemSingleCount : String.localizedStringWithFormat(Strings.PlaylistFolders.playlistFolderSubtitleItemCount, itemCount))",
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
        CPListSection(items: sharedFolderItems),
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

        Task { @MainActor in
          do {
            try await self.initiatePlaybackOfItem(itemId: itemId)
          } catch {
            Logger.module.error("initiatePlaybackOfItem: \(error.localizedDescription)")
            completion()
            return
          }

          guard let listItem = listItem else {
            completion()
            return
          }

          listItem.accessoryType =
            PlaylistManager.shared.state(for: itemId) != .downloaded ? .cloud : .none

          let userInfo = listItem.userInfo as? [String: Any]
          NowPlayingInfo.setNowPlayingMediaArtwork(image: userInfo?["icon"] as? UIImage)
          NowPlayingInfo.updateNowPlayingInfo(self.player)

          completion()

          if self.interfaceController.topTemplate != CPNowPlayingTemplate.shared {
            self.interfaceController.pushTemplate(CPNowPlayingTemplate.shared, animated: true) {
              success,
              error in

              if !success, let error = error {
                Logger.module.error("\(error.localizedDescription)")
              }
            }
          }
        }
      }

      // Update the current playing status
      listItem.isPlaying =
        player.isPlaying && (PlaylistCoordinator.shared.currentPlaylistItem?.src == item.src)

      listItem.accessoryType =
        PlaylistManager.shared.state(for: itemId) != .downloaded ? .cloud : .none
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
            NowPlayingInfo.setNowPlayingMediaArtwork(image: image)
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
        let title =
          enableGridView
          ? Strings.PlayList.playlistCarplayRestartPlaybackButtonStateEnabled
          : Strings.PlayList.playlistCarplayRestartPlaybackButtonStateDisabled
        let icon =
          enableGridView
          ? UIImage(named: "checkbox_on", in: .module, compatibleWith: nil)!
          : UIImage(named: "loginUnselected", in: .module, compatibleWith: nil)!
        let button = CPGridButton(titleVariants: [title], image: icon) { [unowned self] button in
          Preferences.Playlist.enableCarPlayRestartPlayback.value = !enableGridView
          self.interfaceController.popTemplate(
            animated: true,
            completion: { success, error in
              if !success, let error = error {
                Logger.module.error("\(error.localizedDescription)")
              }
            }
          )
        }

        self.interfaceController.pushTemplate(
          CPGridTemplate(
            title: Strings.PlayList.playlistCarplayOptionsScreenTitle,
            gridButtons: [button]
          ),
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
      sectionIndexTitle: Strings.PlayList.playlistCarplayOptionsScreenTitle
    )

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

extension PlaylistLegacyCarplayController: CPInterfaceControllerDelegate {
  func templateWillAppear(_ aTemplate: CPTemplate, animated: Bool) {
    Logger.module.debug("Template \(aTemplate.classForCoder) will appear.")
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

extension PlaylistLegacyCarplayController: NSFetchedResultsControllerDelegate {
  func controller(
    _ controller: NSFetchedResultsController<NSFetchRequestResult>,
    didChange anObject: Any,
    at indexPath: IndexPath?,
    for type: NSFetchedResultsChangeType,
    newIndexPath: IndexPath?
  ) {

    reloadData()
  }

  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    reloadData()
  }
}

extension PlaylistLegacyCarplayController {

  func loadThumbnail(
    for mediaItem: PlaylistInfo,
    completion: @escaping (UIImage?) -> Void
  ) -> PlaylistThumbnailRenderer? {
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
      }
    )
    return thumbnailRenderer
  }

  @MainActor
  func initiatePlaybackOfItem(itemId: String) async throws {
    guard let index = PlaylistManager.shared.index(of: itemId),
      let item = PlaylistManager.shared.itemAtIndex(index)
    else {
      displayLoadingResourceError()
      throw PlaylistCarplayError.invalidItem(id: itemId)
    }

    if !Preferences.Playlist.enableCarPlayRestartPlayback.value {
      // Item is already playing.
      // Show now-playing screen and don't restart playback.
      if PlaylistCoordinator.shared.currentPlaylistItem?.tagId == itemId {
        // If the player is currently paused, un-pause it and play the item.
        // If the player is currently stopped, do nothing.
        if !player.isPlaying, player.currentItem != nil {
          play()
        }
        return
      }
    }

    // Reset Now Playing when playback is starting.
    NowPlayingInfo.clearNowPlayingInfo()

    do {
      PlaylistCoordinator.shared.currentPlaylistItem = item
      PlaylistCoordinator.shared.currentlyPlayingItemIndex = index

      try await playItem(item: item)

      if let item = PlaylistCoordinator.shared.currentPlaylistItem {
        updateLastPlayedItem(item: item)
      }
    } catch {
      PlaylistCoordinator.shared.currentPlaylistItem = nil
      PlaylistCoordinator.shared.currentlyPlayingItemIndex = -1
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

extension PlaylistLegacyCarplayController {
  func onPreviousTrack(isUserInitiated: Bool) {
    if PlaylistCoordinator.shared.currentlyPlayingItemIndex <= 0 {
      return
    }

    let index = PlaylistCoordinator.shared.currentlyPlayingItemIndex - 1
    if index < PlaylistManager.shared.numberOfAssets,
      let item = PlaylistManager.shared.itemAtIndex(index)
    {
      PlaylistCoordinator.shared.currentlyPlayingItemIndex = index

      PlaylistManager.shared.playbackTask = Task { @MainActor in
        do {
          try await playItem(item: item)
          PlaylistCoordinator.shared.currentlyPlayingItemIndex = index
          PlaylistCoordinator.shared.currentPlaylistItem = item
          self.updateLastPlayedItem(item: item)
        } catch {
          PlaylistCoordinator.shared.currentPlaylistItem = nil

          Logger.module.debug("Error Playing Item: \(error)")
          self.displayLoadingResourceError()
        }
      }
    }
  }

  func onNextTrack(isUserInitiated: Bool) {
    let assetCount = PlaylistManager.shared.numberOfAssets
    let isAtEnd = PlaylistCoordinator.shared.currentlyPlayingItemIndex >= assetCount - 1
    var index = PlaylistCoordinator.shared.currentlyPlayingItemIndex

    switch player.repeatState {
    case .none:
      if isAtEnd {
        player.pictureInPictureController?.delegate = nil
        player.pictureInPictureController?.stopPictureInPicture()
        player.pause()

        PlaylistCoordinator.shared.playlistController = nil
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
      let item = PlaylistManager.shared.itemAtIndex(index)
    {
      PlaylistCoordinator.shared.currentPlaylistItem = item
      PlaylistCoordinator.shared.currentlyPlayingItemIndex = index

      PlaylistManager.shared.playbackTask = Task { @MainActor [index] in
        do {
          try await playItem(item: item)
          PlaylistCoordinator.shared.currentlyPlayingItemIndex = index
          PlaylistCoordinator.shared.currentPlaylistItem = item
          self.updateLastPlayedItem(item: item)
        } catch {
          if isUserInitiated || self.player.repeatState == .repeatOne || assetCount <= 1 {
            self.displayLoadingResourceError()
            Logger.module.debug("Error Loading CarPlay Resource: \(error)")
          } else {
            DispatchQueue.main.async {
              PlaylistCoordinator.shared.currentlyPlayingItemIndex = index
              self.onNextTrack(isUserInitiated: isUserInitiated)
            }
          }
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
    NowPlayingInfo.clearNowPlayingInfo()

    PlaylistCoordinator.shared.currentlyPlayingItemIndex = -1
    PlaylistCoordinator.shared.currentPlaylistItem = nil
    player.stop()

    PlaylistManager.shared.playbackTask?.cancel()
    PlaylistManager.shared.playbackTask = nil
  }

  private func clear() {
    NowPlayingInfo.clearNowPlayingInfo()
    player.clear()

    PlaylistManager.shared.playbackTask?.cancel()
    PlaylistManager.shared.playbackTask = nil
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
      let seekTime = CMTimeMakeWithSeconds(
        Float64(
          CGFloat(relativeOffset) * CGFloat(currentItem.asset.duration.value)
            / CGFloat(currentItem.asset.duration.timescale)
        ),
        preferredTimescale: currentItem.currentTime().timescale
      )
      seek(to: seekTime.seconds)
    }
  }

  @MainActor
  func load(url: URL, autoPlayEnabled: Bool) async throws {
    try await load(
      asset: AVURLAsset(url: url, options: AVAsset.defaultOptions),
      autoPlayEnabled: autoPlayEnabled
    )
  }

  @MainActor
  func load(asset: AVURLAsset, autoPlayEnabled: Bool) async throws {
    self.clear()

    let isNewItem = try await player.load(asset: asset)

    // We are playing the same item again..
    if !isNewItem {
      pause()
      seek(relativeOffset: 0.0)  // Restart playback
      play()
      return
    }

    // Track-bar
    if autoPlayEnabled {
      play()  // Play the new item
    }
  }

  @MainActor
  func playItem(item: PlaylistInfo) async throws {
    let isPlaying = player.isPlaying

    // If the item is cached, load it from the cache and play it.
    let cacheState = PlaylistManager.shared.state(for: item.tagId)
    if cacheState != .invalid {
      if let index = PlaylistManager.shared.index(of: item.tagId),
        let asset = PlaylistManager.shared.assetAtIndexSynchronous(index)
      {

        do {
          try await load(asset: asset, autoPlayEnabled: true)
          if !isPlaying {
            NowPlayingInfo.clearNowPlayingInfo()
          }

          NowPlayingInfo.setNowPlayingInfo(item, withPlayer: self.player)
        } catch {
          if !isPlaying {
            NowPlayingInfo.clearNowPlayingInfo()
          }

          throw error
        }
        return
      }

      throw PlaylistMediaStreamer.PlaybackError.expired
    }

    // The item is not cached so we should attempt to stream it
    return try await streamItem(item: item)
  }

  @MainActor
  func streamItem(item: PlaylistInfo) async throws {
    let isPlaying = player.isPlaying
    var item = item

    do {
      item = try await mediaStreamer.loadMediaStreamingAsset(item)
      if !isPlaying { NowPlayingInfo.clearNowPlayingInfo() }
    } catch {
      if !isPlaying { NowPlayingInfo.clearNowPlayingInfo() }
      throw error
    }

    // Item can be streamed
    guard let url = URL(string: item.src)
    else {
      if !isPlaying { NowPlayingInfo.clearNowPlayingInfo() }
      throw PlaylistMediaStreamer.PlaybackError.expired
    }

    // Attempt to play the stream
    do {
      try await load(url: url, autoPlayEnabled: true)
      if !isPlaying { NowPlayingInfo.clearNowPlayingInfo() }
      NowPlayingInfo.setNowPlayingInfo(item, withPlayer: self.player)
      Logger.module.debug("Playing Live Video: \(self.player.isLiveMedia)")
    } catch {
      if !isPlaying { NowPlayingInfo.clearNowPlayingInfo() }
      throw error
    }
  }

  func updateLastPlayedItem(item: PlaylistInfo) {
    guard let playTime = player.currentItem?.currentTime() else {
      return
    }

    PlaylistManager.shared.updateLastPlayed(item: item, playTime: playTime.seconds)
  }

  func displayExpiredResourceError(item: PlaylistInfo?) {
    // CarPlay cannot "open" URLs so we display an alert with an okay button only.
    // Maybe in the future, check if the phone is open, if it is, display the alert there.
    // and the user can "open" the item in the webView/browser.

    if PlaylistLegacyCarplayController.mustUseCPAlertTemplate {
      // Some cars do NOT support CPActionSheetTemplate,
      // So we MUST use CPAlertTemplate
      let alert = CPAlertTemplate(
        titleVariants: [Strings.PlayList.expiredAlertTitle],
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle,
            style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                }
              )
            }
          )
        ]
      )

      interfaceController.presentTemplate(
        alert,
        animated: true,
        completion: { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        }
      )
    } else {
      let alert = CPActionSheetTemplate(
        title: Strings.PlayList.expiredAlertTitle,
        message: Strings.PlayList.expiredAlertDescription,
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle,
            style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                }
              )
            }
          )
        ]
      )

      interfaceController.presentTemplate(
        alert,
        animated: true,
        completion: { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        }
      )
    }
  }

  func displayLoadingResourceError() {
    if PlaylistLegacyCarplayController.mustUseCPAlertTemplate {
      // Some cars do NOT support CPActionSheetTemplate,
      // So we MUST use CPAlertTemplate
      let alert = CPAlertTemplate(
        titleVariants: [Strings.PlayList.loadResourcesErrorAlertDescription],
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle,
            style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                }
              )
            }
          )
        ]
      )

      interfaceController.presentTemplate(
        alert,
        animated: true,
        completion: { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        }
      )
    } else {
      // Can also use CPAlertTemplate, but it doesn't have a "Message" parameter.
      let alert = CPActionSheetTemplate(
        title: Strings.PlayList.sorryAlertTitle,
        message: Strings.PlayList.loadResourcesErrorAlertDescription,
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle,
            style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                }
              )
            }
          )
        ]
      )

      interfaceController.presentTemplate(
        alert,
        animated: true,
        completion: { success, error in
          if !success, let error = error {
            Logger.module.error("\(error.localizedDescription)")
          }
        }
      )
    }
  }

  private func displayErrorAlert(error: Error) {
    // Some cars do NOT support CPActionSheetTemplate
    // So we MUST use CPAlertTemplate
    if PlaylistLegacyCarplayController.mustUseCPAlertTemplate {
      let alert = CPAlertTemplate(
        titleVariants: [error.localizedDescription],
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle,
            style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                }
              )
            }
          )
        ]
      )

      DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
        self.interfaceController.presentTemplate(
          alert,
          animated: true,
          completion: { success, error in
            if !success, let error = error {
              Logger.module.error("\(error.localizedDescription)")
            }
          }
        )
      }
    } else {
      // Can also use CPAlertTemplate, but it doesn't have a "Message" parameter.
      let alert = CPActionSheetTemplate(
        title: Strings.PlayList.sorryAlertTitle,
        message: error.localizedDescription,
        actions: [
          CPAlertAction(
            title: Strings.PlayList.okayButtonTitle,
            style: .default,
            handler: { [weak self] _ in
              self?.interfaceController.dismissTemplate(
                animated: true,
                completion: { success, error in
                  if !success, let error = error {
                    Logger.module.error("\(error.localizedDescription)")
                  }
                }
              )
            }
          )
        ]
      )

      DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
        self.interfaceController.presentTemplate(
          alert,
          animated: true,
          completion: { success, error in
            if !success, let error = error {
              Logger.module.error("\(error.localizedDescription)")
            }
          }
        )
      }
    }
  }
}
