// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import BraveCore
import CarPlay
import Combine
import Data
import Foundation
import MediaPlayer
import Playlist
import PlaylistUI
import Preferences
import Shared
import os.log

/// Coordinates the usage of playlist across the main UI, Picture in Picture and CarPlay
public class PlaylistCoordinator: NSObject {
  private var carPlayStatusObservers = [Any]()
  private(set) weak var mediaPlayer: MediaPlayer?
  private(set) var isCarPlayAvailable = false

  private var carPlayController: Any?
  private var carplayInterface: CPInterfaceController?
  private var carplaySessionConfiguration: CPSessionConfiguration?
  let onCarplayUIChangedToRoot = PassthroughSubject<Void, Never>()

  public weak var browserController: BrowserViewController?

  var currentlyPlayingItemIndex = -1
  var currentPlaylistItem: PlaylistInfo?
  var isPlaylistControllerPresented = false

  // New UI only
  private var playerModel: PlayerModel?
  private var isPiPStarting: Bool = false
  // ----

  // When Picture-In-Picture is enabled, we need to store a reference to the controller to keep it alive, otherwise if it deallocates, the system automatically kills Picture-In-Picture.
  var playlistController: PlaylistViewController? {
    didSet {
      // TODO: REFACTOR and Decide what happens to Playlist in multiple windows in the future
      // IE: Will we show it on each window OR just the one browser controller.
      // After all, we can't play media simultaneously.
      if let selectedTab = browserController?.tabManager.selectedTab,
        let playlistItem = selectedTab.playlistItem,
        PlaylistManager.shared.index(of: playlistItem.tagId) == nil
      {

        // Support for `blob:` Playlist Items
        if playlistItem.src.hasPrefix("blob:")
          && PlaylistManager.shared.allItems.filter({ $0.pageSrc == playlistItem.pageSrc }).first
            != nil
        {
          return
        }

        browserController?.updatePlaylistURLBar(
          tab: selectedTab,
          state: .newItem,
          item: playlistItem
        )
      }
    }
  }

  public func destroyPiP() {
    // This is the only way to have the system kill picture in picture as the restoration controller is deallocated
    // And that means the video is deallocated, its AudioSession is stopped, and the Picture-In-Picture controller is deallocated.
    // This is because `AVPictureInPictureController` is NOT a view controller and there is no way to dismiss it
    // other than to deallocate the restoration controXller.
    // We could also call `AVPictureInPictureController.stopPictureInPicture` BUT we'd still have to deallocate all resources.
    // At least this way, we deallocate both AND pip is stopped in the destructor of `PlaylistViewController->ListController`
    playlistController = nil
  }

  // There can only ever be one instance of this class
  // Because there can only be a single AudioSession and MediaPlayer
  // in use at any given moment
  public static let shared = PlaylistCoordinator()

  func getCarPlayController() -> Any? {
    // On iOS 14, we use CPTemplate (Custom UI)
    // We control what gets displayed
    guard let carplayInterface = carplayInterface else {
      return nil
    }

    // CarPlay can be launched independently of the browser/app
    // CarPlay utilizes the database, and since `SceneDelegate` is never invoked
    // We must manually invoke the database initialization here.
    DataController.shared.initializeOnce()

    // Setup Playlist Download Resume Session
    PlaylistManager.shared.restoreSession()

    // REFACTOR to find a way to get WebKit to load `Youtube` and other sites WITHOUT having to be in the view hierarchy..
    let currentWindow =
      UIApplication.shared.connectedScenes
      .filter({ $0.activationState == .foregroundActive })
      .compactMap({ $0 as? UIWindowScene })
      .first?.windows
      .filter({ $0.isKeyWindow }).first

    let mediaStreamer = PlaylistMediaStreamer(
      playerView: currentWindow ?? UIView(),
      webLoaderFactory: LivePlaylistWebLoaderFactory()
    )

    if FeatureList.kNewPlaylistUI.enabled {
      let player =
        self.playerModel
        ?? PlayerModel(
          mediaStreamer: mediaStreamer,
          initialPlaybackInfo: nil
        )
      self.playerModel = player
      return CarPlayController(player: player, interface: carplayInterface)
    }

    // If there is no media player, create one,
    // pass it to the car-play controller
    let mediaPlayer = self.mediaPlayer ?? MediaPlayer()

    // Construct the CarPlay UI
    let carPlayController = PlaylistLegacyCarplayController(
      mediaStreamer: mediaStreamer,
      player: mediaPlayer,
      interfaceController: carplayInterface
    )
    self.mediaPlayer = mediaPlayer
    return carPlayController
  }

  func getPlaylistController(
    tab: Tab?,
    initialItem: PlaylistInfo?,
    initialItemPlaybackOffset: Double
  ) -> UIViewController {

    // If background playback is enabled (on iPhone), tabs will continue to play media
    // Even if another controller is presented and even when PIP is enabled in playlist.
    // Therefore we need to stop the page/tab from playing when using playlist.
    // On iPad, media will continue to play with or without the background play setting.
    tab?.stopMediaPlayback()

    if FeatureList.kNewPlaylistUI.enabled {
      let mediaStreamer = PlaylistMediaStreamer(
        playerView: browserController!.view,
        webLoaderFactory: LivePlaylistWebLoaderFactory()
      )
      let player =
        self.playerModel
        ?? PlayerModel(
          mediaStreamer: mediaStreamer,
          initialPlaybackInfo: initialItem.map { item in
            .init(itemUUID: item.id, timestamp: initialItemPlaybackOffset)
          }
        )
      player.pictureInPictureDelegate = self
      self.playerModel = player
      return PlaylistHostingController(
        player: player,
        delegate: .init(
          openTabURL: { [weak browserController] url, isPrivate in
            browserController?.dismiss(animated: true)
            browserController?.openURLInNewTab(url, isPrivate: isPrivate, isPrivileged: false)
          },
          onDismissal: { [weak self, weak player] in
            guard let self else { return }
            self.isPlaylistControllerPresented = false
            if let player, !player.isPictureInPictureActive, !isPiPStarting {
              self.destroyPlayerModelIfUnused()
            }
            isPiPStarting = false
          }
        )
      )
    }

    // If there is no media player, create one,
    // pass it to the play-list controller
    let mediaPlayer = self.mediaPlayer ?? MediaPlayer()

    let playlistController =
      self.playlistController
      ?? PlaylistViewController(
        openInNewTab: browserController?.openURLInNewTab,
        openPlaylistSettingsMenu: browserController?.openPlaylistSettingsMenu,
        profile: browserController?.profile,
        mediaPlayer: mediaPlayer,
        initialItem: initialItem,
        initialItemPlaybackOffset: initialItemPlaybackOffset,
        isPrivateBrowsing: browserController?.privateBrowsingManager.isPrivateBrowsing == true
      )
    self.mediaPlayer = mediaPlayer
    return playlistController
  }

  func getPlaylistController(tab: Tab?, completion: @escaping (UIViewController) -> Void) {
    if let playlistController = self.playlistController {
      return completion(playlistController)
    }

    if let tab = tab,
      let item = tab.playlistItem,
      let webView = tab.webView,
      let tag = tab.playlistItem?.tagId
    {
      PlaylistScriptHandler.getCurrentTime(webView: webView, nodeTag: tag) {
        [unowned self] currentTime in
        completion(
          self.getPlaylistController(
            tab: tab,
            initialItem: item,
            initialItemPlaybackOffset: currentTime
          )
        )
      }
    } else {
      return completion(
        getPlaylistController(
          tab: tab,
          initialItem: nil,
          initialItemPlaybackOffset: 0.0
        )
      )
    }
  }

  func pauseAllPlayback() {
    if FeatureList.kNewPlaylistUI.enabled {
      playerModel?.pause()
    } else {
      mediaPlayer?.pause()
    }
  }

  var isPictureInPictureActive: Bool {
    FeatureList.kNewPlaylistUI.enabled
      ? playerModel?.isPictureInPictureActive == true
      : mediaPlayer?.pictureInPictureController?.isPictureInPictureActive == true
  }

  private func destroyPlayerModelIfUnused() {
    guard let playerModel else { return }
    // The player model should stay alive if any of these are true:
    // - Playlist UI is visible
    // - Picture in picture is active
    // - CarPlay is connected
    if !isPlaylistControllerPresented, !playerModel.isPictureInPictureActive, !isCarPlayAvailable {
      playerModel.stop()
      self.playerModel = nil
    }
  }

  private func attemptInterfaceConnection(isCarPlayAvailable: Bool) {
    self.isCarPlayAvailable = isCarPlayAvailable

    // If there is no media player, create one,
    // pass it to the carplay controller
    if isCarPlayAvailable {
      // Protect against reentrancy.
      if carPlayController == nil {
        carPlayController = getCarPlayController()
      }
    } else {
      carPlayController = nil
      mediaPlayer = nil
      destroyPlayerModelIfUnused()
    }

    // Sometimes the `endpointAvailable` WILL RETURN TRUE!
    // Even when the car is NOT connected.
    Logger.module.debug("CARPLAY CONNECTED: \(isCarPlayAvailable)")
  }
}

extension PlaylistCoordinator: CPSessionConfigurationDelegate {
  public func connect(interfaceController: CPInterfaceController) {
    carplayInterface = interfaceController
    carplaySessionConfiguration = CPSessionConfiguration(delegate: self)

    isCarPlayAvailable = true

    DispatchQueue.main.async {
      self.attemptInterfaceConnection(isCarPlayAvailable: true)
    }
  }

  public func disconnect(interfaceController: CPInterfaceController) {
    isCarPlayAvailable = false
    carplayInterface?.delegate = nil
    carplayInterface = nil

    DispatchQueue.main.async {
      self.attemptInterfaceConnection(isCarPlayAvailable: false)
    }
  }

  public func sessionConfiguration(
    _ sessionConfiguration: CPSessionConfiguration,
    limitedUserInterfacesChanged limitedUserInterfaces: CPLimitableUserInterface
  ) {
    Logger.module.debug("Limited UI changed to: \(limitedUserInterfaces.rawValue)")
  }
}

// Handles PiP for new playlist only
extension PlaylistCoordinator: AVPictureInPictureControllerDelegate {
  // This is actually the `restoreUserInterfaceForPictureInPictureStopWithCompletionHandler`
  // delegate method, but the `async` version gets named wierdly
  @MainActor public func pictureInPictureController(
    _ pictureInPictureController: AVPictureInPictureController
  ) async -> Bool {
    guard let browserController else { return false }
    if browserController.presentedViewController is PlaylistHostingController {
      // Already restoring by opening playlist
      return true
    }
    browserController.dismiss(animated: true) {
      browserController.openPlaylist(tab: nil, item: nil)
    }
    return true
  }

  public func pictureInPictureControllerDidStopPictureInPicture(
    _ pictureInPictureController: AVPictureInPictureController
  ) {
    if browserController?.presentedViewController is PlaylistHostingController {
      // Already restoring by opening playlist
      return
    }
    destroyPlayerModelIfUnused()
  }

  public func pictureInPictureControllerWillStartPictureInPicture(
    _ pictureInPictureController: AVPictureInPictureController
  ) {
    // We need to mark that PiP _will_ start, so that when `PlaylistRootView.Delegate.onDismissal`
    // is called we don't kill the player box
    isPiPStarting = true
  }

  public func pictureInPictureControllerDidStartPictureInPicture(
    _ pictureInPictureController: AVPictureInPictureController
  ) {
    // Reset the var tracking this since we can now just look at
    // `AVPictureInPictureController.isPictureInPictureActive`
    isPiPStarting = false
  }
}
