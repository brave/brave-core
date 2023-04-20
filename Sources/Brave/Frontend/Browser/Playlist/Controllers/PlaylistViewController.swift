// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import AVKit
import AVFoundation
import CarPlay
import MediaPlayer
import Combine

import Preferences
import Shared
import SDWebImage
import CoreData
import Data
import os.log

// MARK: PlaylistViewControllerDelegate
protocol PlaylistViewControllerDelegate: AnyObject {
  func attachPlayerView()
  func detachPlayerView()
  func onSidePanelStateChanged()
  func onFullscreen()
  func onExitFullscreen()
  func showStaticImage(image: UIImage?)
  func playItem(item: PlaylistInfo, completion: ((PlaylistInfo, PlaylistMediaStreamer.PlaybackError) -> Void)?)
  func pausePlaying()
  func stopPlaying()
  func deleteItem(itemId: String, at index: Int)
  func updateLastPlayedItem(item: PlaylistInfo)
  func displayLoadingResourceError()
  func displayExpiredResourceError(item: PlaylistInfo)
  func openURLInNewTab(_ url: URL?, isPrivate: Bool, isPrivileged: Bool)
  func openPlaylistSettings()

  var isPlaying: Bool { get }
  var currentPlaylistItem: AVPlayerItem? { get }
  var currentPlaylistAsset: AVAsset? { get }
}

// MARK: PlaylistViewController

class PlaylistViewController: UIViewController {

  // MARK: Properties

  private let player: MediaPlayer
  private let playerView = VideoView()
  private let mediaStreamer: PlaylistMediaStreamer

  private let splitController = UISplitViewController()
  private let folderController = PlaylistFolderController()
  private lazy var listController = PlaylistListViewController(playerView: playerView)
  private let detailController = PlaylistDetailViewController()

  private var lastPlayedTimeObserver: Any?
  private var folderObserver: AnyCancellable?
  private var playerStateObservers = Set<AnyCancellable>()
  private var assetStateObservers = Set<AnyCancellable>()
  private var assetLoadingStateObservers = Set<AnyCancellable>()

  private var openInNewTab: ((_ url: URL?, _ isPrivate: Bool, _ isPrivileged: Bool) -> Void)?
  private var openPlaylistSettingsMenu: (() -> Void)?
  private var folderSharingUrl: String?

  init(
    openInNewTab: ((URL?, Bool, Bool) -> Void)?,
    openPlaylistSettingsMenu: (() -> Void)?,
    profile: Profile?,
    mediaPlayer: MediaPlayer,
    initialItem: PlaylistInfo?,
    initialItemPlaybackOffset: Double
  ) {

    self.openInNewTab = openInNewTab
    self.openPlaylistSettingsMenu = openPlaylistSettingsMenu
    self.player = mediaPlayer
    self.mediaStreamer = PlaylistMediaStreamer(playerView: playerView)
    self.folderSharingUrl = nil
    super.init(nibName: nil, bundle: nil)

    listController.initialItem = initialItem
    listController.initialItemPlaybackOffset = initialItemPlaybackOffset
    listController.delegate = self
    detailController.delegate = self
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    // Store the last played item's time-offset
    if let item = PlaylistCarplayManager.shared.currentPlaylistItem {
      updateLastPlayedItem(item: item)
    }

    // Stop picture in picture
    player.pictureInPictureController?.delegate = nil
    player.pictureInPictureController?.stopPictureInPicture()

    // Simulator cannot "detect" if Car-Play is enabled, therefore we need to STOP playback
    // When this controller deallocates. The user can still manually resume playback in CarPlay.
    if !PlaylistCarplayManager.shared.isCarPlayAvailable {
      // Stop media playback
      stop(playerView)
      PlaylistCarplayManager.shared.currentPlaylistItem = nil
      PlaylistCarplayManager.shared.currentlyPlayingItemIndex = -1

      // Destroy folder observers
      folderObserver = nil
      PlaylistManager.shared.currentFolder = nil
    }

    // Cancel all loading.
    listController.stopLoadingSharedPlaylist()
    assetLoadingStateObservers.removeAll()
    assetStateObservers.removeAll()

    // If this controller is retained in app-delegate for Picture-In-Picture support
    // then we need to re-attach the player layer
    // and deallocate it.
    if UIDevice.isIpad {
      playerView.attachLayer(player: player)
    }

    PlaylistCarplayManager.shared.playlistController = nil
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    overrideUserInterfaceStyle = .dark

    // Setup delegates and state observers
    attachPlayerView()
    updatePlayerUI()
    observePlayerStates()
    observeFolderStates()
    listController.delegate = self

    // Layout
    splitController.do {
      $0.viewControllers = [
        SettingsNavigationController(rootViewController: folderController),
        SettingsNavigationController(rootViewController: detailController),
      ]
      $0.delegate = self
      $0.primaryEdge = PlayListSide(rawValue: Preferences.Playlist.listViewSide.value) == .left ? .leading : .trailing
      $0.presentsWithGesture = false
      $0.maximumPrimaryColumnWidth = 400
      $0.minimumPrimaryColumnWidth = 400
    }
    
    if let folderSharingUrl = folderSharingUrl {
      if let navigationController = folderController.navigationController {
        navigationController.popToRootViewController(animated: false)
        splitController.preferredDisplayMode = .oneOverSecondary
        listController.loadingState = .loading
        PlaylistManager.shared.currentFolder = nil
        navigationController.pushViewController(listController, animated: false)
      }
      
      setFolderSharingUrl(folderSharingUrl)
    } else if let initialItem = listController.initialItem,
      let item = PlaylistItem.getItem(uuid: initialItem.tagId) {
      listController.loadingState = .fullyLoaded
      PlaylistManager.shared.currentFolder = item.playlistFolder
    } else if let url = Preferences.Playlist.lastPlayedItemUrl.value,
              let item = PlaylistItem.getItems(pageSrc: url).first {
      listController.loadingState = .fullyLoaded
      PlaylistManager.shared.currentFolder = item.playlistFolder
    } else {
      listController.loadingState = .fullyLoaded
      PlaylistManager.shared.currentFolder = nil
    }

    if PlaylistManager.shared.currentFolder != nil {
      folderController.navigationController?.pushViewController(listController, animated: false)
    }

    addChild(splitController)
    view.addSubview(splitController.view)

    splitController.do {
      $0.didMove(toParent: self)
      $0.view.translatesAutoresizingMaskIntoConstraints = false
      $0.view.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }
    }

    // Updates
    updateLayoutForOrientationChange()

    detailController.setVideoPlayer(playerView)
    updateLayoutForOrientationMode()
  }
  
  func setFolderSharingUrl(_ folderSharingUrl: String) {
    self.folderSharingUrl = folderSharingUrl
    
    if presentingViewController != nil || navigationController != nil || parent != nil {
      self.folderSharingUrl = nil
      listController.loadSharedPlaylist(folderSharingUrl: folderSharingUrl)
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)

    folderObserver = PlaylistManager.shared.onCurrentFolderDidChange
      .receive(on: DispatchQueue.main)
      .sink { [weak self] in
        guard let self = self,
          self.listController.parent == nil,
          PlaylistManager.shared.currentFolder != nil
        else { return }

        self.updateLayoutForOrientationMode()
        self.folderController.navigationController?.pushViewController(self.listController, animated: true)
      }
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)

    folderObserver = nil
  }

  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)

    updateLayoutForOrientationChange()
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    return .lightContent
  }

  private func updateLayoutForOrientationMode() {
    detailController.navigationController?.setNavigationBarHidden(splitController.isCollapsed || traitCollection.horizontalSizeClass == .regular, animated: false)

    if UIDevice.isPhone {
      if splitController.isCollapsed == false && traitCollection.horizontalSizeClass == .regular {
        listController.updateLayoutForMode(.pad)
        detailController.updateLayoutForMode(.pad)
      } else {
        listController.updateLayoutForMode(.phone)
        detailController.updateLayoutForMode(.phone)

        // On iPhone Pro Max which displays like an iPad, we need to hide navigation bar.
        if UIDevice.isPhone && UIDevice.current.orientation.isLandscape {
          listController.onFullscreen()
        }
      }
    } else {
      listController.updateLayoutForMode(.pad)
      detailController.updateLayoutForMode(.pad)
    }
  }

  private func updateLayoutForOrientationChange() {
    if playerView.isFullscreen {
      splitController.preferredDisplayMode = .secondaryOnly
    } else {
      if UIDevice.current.orientation.isLandscape {
        splitController.preferredDisplayMode = PlaylistManager.shared.currentFolder?.isPersistent == true ? .oneOverSecondary : .secondaryOnly
      } else {
        splitController.preferredDisplayMode = .oneOverSecondary
      }
    }
  }

  private func updatePlayerUI() {
    // Update play/pause button
    if isPlaying {
      playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!, for: .normal)
    } else {
      playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!, for: .normal)
    }

    // Update play-backrate button
    let playbackRate = player.rate
    let button = playerView.controlsView.playbackRateButton

    if playbackRate <= 1.0 {
      button.setTitle("1x", for: .normal)
    } else if playbackRate == 1.5 {
      button.setTitle("1.5x", for: .normal)
    } else {
      button.setTitle("2x", for: .normal)
    }

    // Update repeatMode button
    switch repeatMode {
    case .none:
      playerView.controlsView.repeatButton.setImage(UIImage(named: "playlist_repeat", in: .module, compatibleWith: nil)!, for: .normal)
    case .repeatOne:
      playerView.controlsView.repeatButton.setImage(UIImage(named: "playlist_repeat_one", in: .module, compatibleWith: nil)!, for: .normal)
    case .repeatAll:
      playerView.controlsView.repeatButton.setImage(UIImage(named: "playlist_repeat_all", in: .module, compatibleWith: nil)!, for: .normal)
    }

    if let item = PlaylistCarplayManager.shared.currentPlaylistItem {
      playerView.setVideoInfo(videoDomain: item.pageSrc, videoTitle: item.pageTitle)
    } else {
      playerView.resetVideoInfo()
    }
  }

  private func observePlayerStates() {
    player.publisher(for: .play).sink { [weak self] event in
      guard let self = self else { return }
      self.playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!, for: .normal)

      if !PlaylistCarplayManager.shared.isCarPlayAvailable {
        MPNowPlayingInfoCenter.default().playbackState = .playing
        PlaylistMediaStreamer.updateNowPlayingInfo(event.mediaPlayer)
      } else if let item = PlaylistCarplayManager.shared.currentPlaylistItem {
        self.playerView.setVideoInfo(
          videoDomain: item.pageSrc,
          videoTitle: item.pageTitle)
      }
      
      self.listController.highlightActiveItem()
    }.store(in: &playerStateObservers)

    player.publisher(for: .pause).sink { [weak self] event in
      self?.playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!, for: .normal)

      if !PlaylistCarplayManager.shared.isCarPlayAvailable {
        MPNowPlayingInfoCenter.default().playbackState = .paused
        PlaylistMediaStreamer.updateNowPlayingInfo(event.mediaPlayer)
      }
    }.store(in: &playerStateObservers)

    player.publisher(for: .stop).sink { [weak self] _ in
      guard let self = self else { return }
      self.playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!, for: .normal)
      self.playerView.resetVideoInfo()
      PlaylistMediaStreamer.clearNowPlayingInfo()

      PlaylistCarplayManager.shared.currentlyPlayingItemIndex = -1
      PlaylistCarplayManager.shared.currentPlaylistItem = nil

      // Cancel all loading.
      self.assetLoadingStateObservers.removeAll()
      self.assetStateObservers.removeAll()
    }.store(in: &playerStateObservers)

    player.publisher(for: .changePlaybackRate).sink { [weak self] event in
      guard let self = self else { return }

      let playbackRate = event.mediaPlayer.rate
      let button = self.playerView.controlsView.playbackRateButton

      if playbackRate <= 1.0 {
        button.setTitle("1x", for: .normal)
      } else if playbackRate == 1.5 {
        button.setTitle("1.5x", for: .normal)
      } else if playbackRate == 2.0 {
        button.setTitle("2.0x", for: .normal)
      } else {
        button.setTitle("2.5x", for: .normal)
      }

      if !PlaylistCarplayManager.shared.isCarPlayAvailable {
        MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackRate] = event.mediaPlayer.rate
      }
    }.store(in: &playerStateObservers)

    player.publisher(for: .changeRepeatMode).sink { [weak self] _ in
      guard let self = self else { return }
      switch self.repeatMode {
      case .none:
        self.playerView.controlsView.repeatButton.setImage(UIImage(named: "playlist_repeat", in: .module, compatibleWith: nil)!, for: .normal)
      case .repeatOne:
        self.playerView.controlsView.repeatButton.setImage(UIImage(named: "playlist_repeat_one", in: .module, compatibleWith: nil)!, for: .normal)
      case .repeatAll:
        self.playerView.controlsView.repeatButton.setImage(UIImage(named: "playlist_repeat_all", in: .module, compatibleWith: nil)!, for: .normal)
      }
    }.store(in: &playerStateObservers)
    
    player.publisher(for: .previousTrack).sink { [weak self] _ in
      guard let self = self, !PlaylistCarplayManager.shared.isCarPlayAvailable else { return }
      self.onPreviousTrack(self.playerView, isUserInitiated: true)
    }.store(in: &playerStateObservers)

    player.publisher(for: .nextTrack).sink { [weak self] _ in
      guard let self = self, !PlaylistCarplayManager.shared.isCarPlayAvailable else { return }
      self.onNextTrack(self.playerView, isUserInitiated: true)
    }.store(in: &playerStateObservers)

    player.publisher(for: .finishedPlaying).sink { [weak self] event in
      guard let self = self,
        let currentItem = event.mediaPlayer.currentItem
      else { return }

      // When CarPlay is available, do NOT pause or handle `nextTrack`
      // CarPlay will do all of that. So, just update the UI only.
      if PlaylistCarplayManager.shared.isCarPlayAvailable {
        self.playerView.controlsView.playPauseButton.isEnabled = false
        self.playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!, for: .normal)

        let endTime = CMTimeConvertScale(currentItem.asset.duration, timescale: event.mediaPlayer.currentTime.timescale, method: .roundHalfAwayFromZero)

        self.playerView.controlsView.trackBar.setTimeRange(currentTime: currentItem.currentTime(), endTime: endTime)
        event.mediaPlayer.seek(to: .zero)

        self.playerView.controlsView.playPauseButton.isEnabled = true
        self.playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!, for: .normal)

        self.playerView.toggleOverlays(showOverlay: true)
      } else {
        var nowPlayingInfo = MPNowPlayingInfoCenter.default().nowPlayingInfo
        nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackProgress] = 0.0
        nowPlayingInfo?[MPNowPlayingInfoPropertyElapsedPlaybackTime] = 0.0
        nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackRate] = 0.0
        MPNowPlayingInfoCenter.default().nowPlayingInfo = nowPlayingInfo

        self.playerView.controlsView.playPauseButton.isEnabled = false
        self.playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!, for: .normal)

        event.mediaPlayer.pause()

        let endTime = CMTimeConvertScale(currentItem.asset.duration, timescale: event.mediaPlayer.currentTime.timescale, method: .roundHalfAwayFromZero)

        self.playerView.controlsView.trackBar.setTimeRange(currentTime: currentItem.currentTime(), endTime: endTime)
        event.mediaPlayer.seek(to: .zero)
        
        if let item = PlaylistCarplayManager.shared.currentPlaylistItem {
          self.updateLastPlayedItem(item: item)
        }

        self.playerView.controlsView.playPauseButton.isEnabled = true
        self.playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!, for: .normal)

        self.playerView.toggleOverlays(showOverlay: true)
        self.onNextTrack(self.playerView, isUserInitiated: false)
      }
    }.store(in: &playerStateObservers)

    player.publisher(for: .periodicPlayTimeChanged).sink { [weak self] event in
      guard let self = self, let currentItem = event.mediaPlayer.currentItem else { return }

      let endTime = CMTimeConvertScale(currentItem.asset.duration, timescale: event.mediaPlayer.currentTime.timescale, method: .roundHalfAwayFromZero)

      if CMTimeCompare(endTime, .zero) != 0 && endTime.value > 0 {
        self.playerView.controlsView.trackBar.setTimeRange(currentTime: event.mediaPlayer.currentTime, endTime: endTime)
      }
    }.store(in: &playerStateObservers)

    self.playerView.infoView.pictureInPictureButton.isEnabled =
      AVPictureInPictureController.isPictureInPictureSupported()
    player.publisher(for: .pictureInPictureStatusChanged).sink { [weak self] event in
      guard let self = self else { return }

      self.playerView.infoView.pictureInPictureButton.isEnabled =
        event.mediaPlayer.pictureInPictureController?.isPictureInPicturePossible == true
    }.store(in: &playerStateObservers)
    
    lastPlayedTimeObserver = self.player.addTimeObserver(interval: 5000, onTick: { [weak self] _ in
      if let currentItem = PlaylistCarplayManager.shared.currentPlaylistItem {
        self?.updateLastPlayedItem(item: currentItem)
      }
    })
  }

  private func observeFolderStates() {
    folderController.onFolderSelected = { folder in
      PlaylistManager.shared.currentFolder = folder
    }
  }
}

// MARK: - UIAdaptivePresentationControllerDelegate

extension PlaylistViewController: UIAdaptivePresentationControllerDelegate {
  func adaptivePresentationStyle(for controller: UIPresentationController, traitCollection: UITraitCollection) -> UIModalPresentationStyle {
    return .fullScreen
  }
}

// MARK: - UISplitViewControllerDelegate

extension PlaylistViewController: UISplitViewControllerDelegate {

  func splitViewControllerSupportedInterfaceOrientations(_ splitViewController: UISplitViewController) -> UIInterfaceOrientationMask {
    return .allButUpsideDown
  }

  func splitViewController(_ splitViewController: UISplitViewController, collapseSecondary secondaryViewController: UIViewController, onto primaryViewController: UIViewController) -> Bool {

    // On iPhone, always display the iPhone layout (collapsed) no matter what.
    // On iPad, we need to update both the list controller's layout (collapsed) and the detail controller's layout (collapsed).
    listController.updateLayoutForMode(.phone)
    detailController.setVideoPlayer(nil)
    detailController.updateLayoutForMode(.phone)
    return true
  }

  func splitViewController(_ splitViewController: UISplitViewController, separateSecondaryFrom primaryViewController: UIViewController) -> UIViewController? {

    // On iPhone, always display the iPad layout (expanded) when not in compact mode.
    // On iPad, we need to update both the list controller's layout (expanded) and the detail controller's layout (expanded).
    listController.updateLayoutForMode(.pad)
    detailController.setVideoPlayer(playerView)
    detailController.updateLayoutForMode(.pad)

    if UIDevice.isPhone {
      detailController.navigationController?.setNavigationBarHidden(true, animated: true)
    }

    return detailController.navigationController ?? detailController
  }
}

// MARK: - PlaylistViewControllerDelegate

extension PlaylistViewController: PlaylistViewControllerDelegate {
  func attachPlayerView() {
    playerView.delegate = self
    playerView.attachLayer(player: player)
  }

  func detachPlayerView() {
    playerView.delegate = nil
    playerView.detachLayer()
  }

  func onSidePanelStateChanged() {
    detailController.onSidePanelStateChanged()
  }

  func onFullscreen() {
    if !UIDevice.isIpad || splitController.isCollapsed {
      navigationController?.setToolbarHidden(true, animated: false)
      listController.onFullscreen()
    } else {
      detailController.onFullscreen()
    }
    
    setNeedsUpdateOfHomeIndicatorAutoHidden()
  }
  
  override var prefersHomeIndicatorAutoHidden: Bool {
    if !UIDevice.isIpad || splitController.isCollapsed {
      return listController.tableView.isHidden
    } else {
      return detailController.navigationController?.isNavigationBarHidden == true
    }
  }

  func onExitFullscreen() {
    if !UIDevice.isIpad || splitController.isCollapsed {
      listController.onExitFullscreen()
    } else {
      if playerView.isFullscreen {
        detailController.onExitFullscreen()
      } else {
        dismiss(animated: true, completion: nil)
      }
    }
    setNeedsUpdateOfHomeIndicatorAutoHidden()
  }
  
  func showStaticImage(image: UIImage?) {
    playerView.setStaticImage(image: image)
  }

  func pausePlaying() {
    playerView.pause()
  }

  func stopPlaying() {
    PlaylistMediaStreamer.clearNowPlayingInfo()

    PlaylistCarplayManager.shared.currentlyPlayingItemIndex = -1
    PlaylistCarplayManager.shared.currentPlaylistItem = nil
    playerView.resetVideoInfo()
    stop(playerView)

    // Cancel all loading.
    assetLoadingStateObservers.removeAll()
    assetStateObservers.removeAll()
  }

  func deleteItem(itemId: String, at index: Int) {
    guard let item = PlaylistItem.getItem(uuid: itemId) else {
      return
    }
    
    PlaylistManager.shared.delete(item: PlaylistInfo(item: item))

    if PlaylistCarplayManager.shared.currentlyPlayingItemIndex == index || PlaylistManager.shared.numberOfAssets == 0 {
      stopPlaying()
    }
  }

  func updateLastPlayedItem(item: PlaylistInfo) {
    Preferences.Playlist.lastPlayedItemUrl.value = item.pageSrc
    
    guard let playTime = player.currentItem?.currentTime() else {
      return
    }
    
    let lastPlayedTime = Preferences.Playlist.playbackLeftOff.value ? playTime.seconds : 0.0
    PlaylistItem.updateLastPlayed(itemId: item.tagId, pageSrc: item.pageSrc, lastPlayedOffset: lastPlayedTime)
  }

  func displayLoadingResourceError() {
    let isPrimaryDisplayMode = splitController.preferredDisplayMode == .oneOverSecondary
    if isPrimaryDisplayMode {
      listController.displayLoadingResourceError()
    } else {
      detailController.displayLoadingResourceError()
    }
  }

  func displayExpiredResourceError(item: PlaylistInfo) {
    let isPrimaryDisplayMode = splitController.preferredDisplayMode == .oneOverSecondary
    if isPrimaryDisplayMode {
      listController.displayExpiredResourceError(item: item)
    } else {
      detailController.displayExpiredResourceError(item: item)
    }
  }

  func openURLInNewTab(_ url: URL?, isPrivate: Bool, isPrivileged: Bool) {
    openInNewTab?(url, isPrivate, isPrivileged)
  }
  
  func openPlaylistSettings() {
    let openMenu = openPlaylistSettingsMenu
    self.dismiss(animated: false) {
      DispatchQueue.main.async {
        openMenu?()
      }
    }
  }

  var currentPlaylistItem: AVPlayerItem? {
    player.currentItem
  }

  var currentPlaylistAsset: AVAsset? {
    player.currentItem?.asset
  }
}

// MARK: - VideoViewDelegate

extension PlaylistViewController: VideoViewDelegate {
  func onSidePanelStateChanged(_ videoView: VideoView) {
    onSidePanelStateChanged()
  }

  func onPreviousTrack(_ videoView: VideoView, isUserInitiated: Bool) {
    if PlaylistCarplayManager.shared.currentlyPlayingItemIndex <= 0 {
      return
    }

    let index = PlaylistCarplayManager.shared.currentlyPlayingItemIndex - 1
    if index < PlaylistManager.shared.numberOfAssets {
      let indexPath = IndexPath(row: index, section: 0)
      listController.prepareToPlayItem(at: indexPath) { [weak self] item in
        guard let self = self,
          let item = item
        else {

          self?.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
          return
        }

        PlaylistCarplayManager.shared.currentlyPlayingItemIndex = indexPath.row
        PlaylistCarplayManager.shared.currentPlaylistItem = item

        self.playItem(item: item) { [weak self] item, error in
          PlaylistCarplayManager.shared.currentPlaylistItem = nil

          guard let self = self else { return }
          PlaylistCarplayManager.shared.currentPlaylistItem = item

          switch error {
          case .other(let err):
            Logger.module.error("\(err.localizedDescription)")
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            self.displayLoadingResourceError()
          case .cannotLoadMedia:
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            self.displayLoadingResourceError()
          case .expired:
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: true)
            self.displayExpiredResourceError(item: item)
          case .none:
            PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
            PlaylistCarplayManager.shared.currentPlaylistItem = item
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            self.updateLastPlayedItem(item: item)
          case .cancelled:
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            Logger.module.debug("User Cancelled Playlist Playback")
          }
        }
      }
    }
  }

  func onNextTrack(_ videoView: VideoView, isUserInitiated: Bool) {
    let assetCount = PlaylistManager.shared.numberOfAssets
    let isAtEnd = PlaylistCarplayManager.shared.currentlyPlayingItemIndex >= assetCount - 1
    var index = PlaylistCarplayManager.shared.currentlyPlayingItemIndex

    switch repeatMode {
    case .none:
      if isAtEnd {
        player.pictureInPictureController?.delegate = nil
        player.pictureInPictureController?.stopPictureInPicture()
        player.stop()

        if UIDevice.isIpad {
          playerView.attachLayer(player: player)
        }
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

    if index >= 0 {
      let indexPath = IndexPath(row: index, section: 0)
      
      listController.highlightActiveItem()
      listController.prepareToPlayItem(at: indexPath) { [weak self] item in
        guard let self = self,
          let item = item
        else {

          self?.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
          return
        }

        PlaylistCarplayManager.shared.currentlyPlayingItemIndex = indexPath.row
        PlaylistCarplayManager.shared.currentPlaylistItem = item

        self.playItem(item: item) { [weak self] item, error in
          PlaylistCarplayManager.shared.currentPlaylistItem = nil
          guard let self = self else { return }
          PlaylistCarplayManager.shared.currentPlaylistItem = item

          switch error {
          case .other(let err):
            Logger.module.error("\(err.localizedDescription)")
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            self.displayLoadingResourceError()
          case .cannotLoadMedia:
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            self.displayLoadingResourceError()
          case .expired:
            if isUserInitiated || self.repeatMode == .repeatOne || assetCount <= 1 {
              self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: true)
              self.displayExpiredResourceError(item: item)
            } else {
              DispatchQueue.main.async {
                self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
                PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
                self.onNextTrack(videoView, isUserInitiated: isUserInitiated)
              }
            }
          case .none:
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            PlaylistCarplayManager.shared.currentlyPlayingItemIndex = index
            PlaylistCarplayManager.shared.currentPlaylistItem = item
            self.updateLastPlayedItem(item: item)
          case .cancelled:
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            Logger.module.debug("User Cancelled Playlist Playback")
          }
        }
      }
    }
  }

  func onPictureInPicture(_ videoView: VideoView) {
    guard let pictureInPictureController = player.pictureInPictureController else { return }

    DispatchQueue.main.async {
      if pictureInPictureController.isPictureInPictureActive {
        // Picture in Picture disabled
        pictureInPictureController.delegate = self
        pictureInPictureController.stopPictureInPicture()
      } else {
        pictureInPictureController.requiresLinearPlayback = false
        // Picture in Picture enabled
        pictureInPictureController.delegate = self
        pictureInPictureController.startPictureInPicture()
      }
    }
  }

  func onFullscreen(_ videoView: VideoView) {
    onFullscreen()
  }

  func onExitFullscreen(_ videoView: VideoView) {
    onExitFullscreen()
  }

  func onFavIconSelected(_ videoView: VideoView) {
    listController.onFavIconSelected(videoView)
  }

  func play(_ videoView: VideoView) {
    if isPlaying {
      playerView.toggleOverlays(showOverlay: playerView.isOverlayDisplayed)
    } else {
      playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!, for: .normal)
      playerView.toggleOverlays(showOverlay: false)
      playerView.isOverlayDisplayed = false

      player.play()
    }
  }

  func pause(_ videoView: VideoView) {
    if isPlaying {
      playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!, for: .normal)
      playerView.toggleOverlays(showOverlay: true)
      playerView.isOverlayDisplayed = true

      player.pause()
    } else {
      playerView.toggleOverlays(showOverlay: playerView.isOverlayDisplayed)
    }
  }

  func stop(_ videoView: VideoView) {
    playerView.controlsView.playPauseButton.setImage(UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!, for: .normal)
    playerView.toggleOverlays(showOverlay: true)
    playerView.isOverlayDisplayed = true

    player.stop()
  }

  func seekBackwards(_ videoView: VideoView) {
    player.seekBackwards()
  }

  func seekForwards(_ videoView: VideoView) {
    player.seekForwards()
  }

  func seek(_ videoView: VideoView, to time: TimeInterval) {
    player.seek(to: time)
  }

  func seek(_ videoView: VideoView, relativeOffset: Float) {
    if let currentItem = player.currentItem {
      let seekTime = CMTimeMakeWithSeconds(Float64(CGFloat(relativeOffset) * CGFloat(currentItem.asset.duration.value) / CGFloat(currentItem.asset.duration.timescale)), preferredTimescale: currentItem.currentTime().timescale)
      seek(videoView, to: seekTime.seconds)
    }
  }
  
  func setPlaybackRate(_ videoView: VideoView, rate: Float) {
    player.setPlaybackRate(rate: rate)
  }

  func togglePlayerGravity(_ videoView: VideoView) {
    player.toggleGravity()
  }

  func toggleRepeatMode(_ videoView: VideoView) {
    player.toggleRepeatMode()
  }

  func load(_ videoView: VideoView, url: URL, autoPlayEnabled: Bool) -> AnyPublisher<Void, MediaPlaybackError> {
    load(videoView, asset: AVURLAsset(url: url, options: AVAsset.defaultOptions), autoPlayEnabled: autoPlayEnabled)
  }

  func load(_ videoView: VideoView, asset: AVURLAsset, autoPlayEnabled: Bool) -> AnyPublisher<Void, MediaPlaybackError> {
    assetLoadingStateObservers.removeAll()
    player.stop()

    return Future { [weak self] resolver in
      guard let self = self else {
        Logger.module.debug("User Cancelled Playback")
        resolver(.failure(.cancelled))
        return
      }

      self.player.load(asset: asset)
        .receive(on: RunLoop.main)
        .sink(
          receiveCompletion: { status in
            switch status {
            case .failure(let error):
              switch error {
              case .cancelled:
                resolver(.failure(.cancelled))
              case .other(let err):
                resolver(.failure(.other(err)))
              case .cannotLoadAsset:
                resolver(.failure(.other(error)))
              }
            case .finished:
              break
            }
          },
          receiveValue: { [weak self] isNewItem in
            guard let self = self, let item = self.player.currentItem else {
              Logger.module.debug("User Cancelled Playback")
              resolver(.failure(.cancelled))
              return
            }

            // We are playing the same item again..
            if !isNewItem {
              self.pause(videoView)
              self.seek(videoView, relativeOffset: 0.0)  // Restart playback
              self.play(videoView)
              resolver(.success(Void()))
              return
            }

            // Live media item
            let isPlayingLiveMedia = self.player.isLiveMedia
            self.playerView.setMediaIsLive(isPlayingLiveMedia)

            // Track-bar
            let endTime = CMTimeConvertScale(item.asset.duration, timescale: self.player.currentTime.timescale, method: .roundHalfAwayFromZero)
            self.playerView.controlsView.trackBar.setTimeRange(currentTime: item.currentTime(), endTime: endTime)

            // Successfully loaded
            resolver(.success(Void()))

            if autoPlayEnabled {
              self.play(videoView)  // Play the new item
            }
          }
        ).store(in: &self.assetLoadingStateObservers)
    }.eraseToAnyPublisher()
  }

  func playItem(item: PlaylistInfo, completion: ((PlaylistInfo, PlaylistMediaStreamer.PlaybackError) -> Void)?) {
    assetLoadingStateObservers.removeAll()
    assetStateObservers.removeAll()

    // This MUST be checked.
    // The user must not be able to alter a player that isn't visible from any UI!
    // This is because, if car-play is interface is attached, the player can only be
    // controller through this UI so long as it is attached to it.
    // If it isn't attached, the player can only be controlled through the car-play interface.
    guard player.isAttachedToDisplay else {
      completion?(item, .cancelled)
      return
    }

    // If the item is cached, load it from the cache and play it.
    let cacheState = PlaylistManager.shared.state(for: item.tagId)
    if cacheState != .invalid {
      if let index = PlaylistManager.shared.index(of: item.tagId),
        let asset = PlaylistManager.shared.assetAtIndex(index) {
        load(playerView, asset: asset, autoPlayEnabled: listController.autoPlayEnabled)
          .handleEvents(receiveCancel: {
            PlaylistMediaStreamer.clearNowPlayingInfo()
            completion?(item, .cancelled)
          })
          .sink(
            receiveCompletion: { status in
              switch status {
              case .failure(let error):
                PlaylistMediaStreamer.clearNowPlayingInfo()

                switch error {
                case .cancelled:
                  completion?(item, .cancelled)
                case .other(let err):
                  completion?(item, .other(err))
                case .cannotLoadAsset(_):
                  completion?(item, .other(error))
                }

              case .finished:
                break
              }
            },
            receiveValue: { [weak self] _ in
              guard let self = self else {
                PlaylistMediaStreamer.clearNowPlayingInfo()
                completion?(item, .cancelled)
                return
              }

              self.playerView.setVideoInfo(
                videoDomain: item.pageSrc,
                videoTitle: item.pageTitle)
              PlaylistMediaStreamer.setNowPlayingInfo(item, withPlayer: self.player)
              completion?(item, .none)
            }
          ).store(in: &assetLoadingStateObservers)
      } else {
        PlaylistMediaStreamer.clearNowPlayingInfo()
        completion?(item, .expired)
      }
      return
    }

    // The item is not cached so we should attempt to stream it
    streamItem(item: item, completion: completion)
  }

  func streamItem(item: PlaylistInfo, completion: ((PlaylistInfo, PlaylistMediaStreamer.PlaybackError) -> Void)?) {
    mediaStreamer.loadMediaStreamingAsset(item)
      .handleEvents(receiveCancel: {
        PlaylistMediaStreamer.clearNowPlayingInfo()
        completion?(item, .cancelled)
      })
      .sink(
        receiveCompletion: { status in
          switch status {
          case .failure(let error):
            PlaylistMediaStreamer.clearNowPlayingInfo()
            completion?(item, error)
          case .finished:
            break
          }
        },
        receiveValue: { [weak self] item in
          guard let self = self else {
            PlaylistMediaStreamer.clearNowPlayingInfo()
            completion?(item, .cancelled)
            return
          }

          // Item can be streamed, so let's retrieve its URL from our DB
          guard let index = PlaylistManager.shared.index(of: item.tagId),
            let item = PlaylistManager.shared.itemAtIndex(index)
          else {
            PlaylistMediaStreamer.clearNowPlayingInfo()
            completion?(item, .expired)
            return
          }

          // Attempt to play the stream
          if let url = URL(string: item.src) {
            self.load(
              self.playerView,
              url: url,
              autoPlayEnabled: self.listController.autoPlayEnabled
            )
            .handleEvents(receiveCancel: {
              PlaylistMediaStreamer.clearNowPlayingInfo()
              completion?(item, .cancelled)
            })
            .sink(
              receiveCompletion: { status in
                switch status {
                case .failure(let error):
                  PlaylistMediaStreamer.clearNowPlayingInfo()

                  switch error {
                  case .cancelled:
                    completion?(item, .cancelled)
                  case .other(let err):
                    completion?(item, .other(err))
                  case .cannotLoadAsset(_):
                    completion?(item, .other(error))
                  }
                case .finished:
                  break
                }
              },
              receiveValue: { [weak self] _ in
                guard let self = self else {
                  PlaylistMediaStreamer.clearNowPlayingInfo()
                  completion?(item, .cancelled)
                  return
                }

                self.playerView.setVideoInfo(
                  videoDomain: item.pageSrc,
                  videoTitle: item.pageTitle)
                PlaylistMediaStreamer.setNowPlayingInfo(item, withPlayer: self.player)
                completion?(item, .none)
              }
            ).store(in: &self.assetLoadingStateObservers)
          } else {
            PlaylistMediaStreamer.clearNowPlayingInfo()
            completion?(item, .expired)
          }
        }
      ).store(in: &assetStateObservers)
  }

  var isPlaying: Bool {
    player.isPlaying
  }

  var repeatMode: MediaPlayer.RepeatMode {
    player.repeatState
  }

  var playbackRate: Float {
    return player.rate
  }

  var isVideoTracksAvailable: Bool {
    if let item = player.currentItem {
      return item.isVideoTracksAvailable()
    }

    // We do this because for m3u8 HLS streams,
    // tracks may not always be available and the particle effect will show even on videos..
    // It's best to assume this type of media is a video stream.
    return true
  }
}

extension PlaylistFolder {
  var isPersistent: Bool {
    managedObjectContext?.persistentStoreCoordinator?.persistentStores.first(where: {
      return $0.type == NSPersistentStore.StoreType.inMemory.rawValue
    }) == nil
  }
}
