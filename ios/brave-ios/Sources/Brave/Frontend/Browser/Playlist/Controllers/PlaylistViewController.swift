// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import AVKit
import CarPlay
import Combine
import CoreData
import Data
import Foundation
import MediaPlayer
import Playlist
import Preferences
import SDWebImage
import Shared
import UIKit
import os.log

// MARK: PlaylistViewControllerDelegate
protocol PlaylistViewControllerDelegate: AnyObject {
  func attachPlayerView()
  func onSidePanelStateChanged()
  func onFullscreen()
  func onExitFullscreen()
  func showStaticImage(image: UIImage?)
  func playItem(item: PlaylistInfo) async throws -> PlaylistInfo
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
  private let isPrivateBrowsing: Bool

  private let splitController = UISplitViewController()
  private let folderController = PlaylistFolderController()
  private lazy var listController = PlaylistListViewController(
    playerView: playerView,
    isPrivateBrowsing: isPrivateBrowsing
  )
  private lazy var detailController = PlaylistDetailViewController(
    isPrivateBrowsing: isPrivateBrowsing
  )

  private var folderObserver: AnyCancellable?
  private var playerStateObservers = Set<AnyCancellable>()

  private var openInNewTab: ((_ url: URL?, _ isPrivate: Bool, _ isPrivileged: Bool) -> Void)?
  private var openPlaylistSettingsMenu: (() -> Void)?
  private var folderSharingUrl: String?

  init(
    openInNewTab: ((URL?, Bool, Bool) -> Void)?,
    openPlaylistSettingsMenu: (() -> Void)?,
    profile: Profile?,
    mediaPlayer: MediaPlayer,
    initialItem: PlaylistInfo?,
    initialItemPlaybackOffset: Double,
    isPrivateBrowsing: Bool
  ) {

    self.openInNewTab = openInNewTab
    self.openPlaylistSettingsMenu = openPlaylistSettingsMenu
    self.player = mediaPlayer
    self.mediaStreamer = PlaylistMediaStreamer(
      playerView: playerView,
      webLoaderFactory: LivePlaylistWebLoaderFactory()
    )
    self.isPrivateBrowsing = isPrivateBrowsing
    self.folderSharingUrl = nil

    super.init(nibName: nil, bundle: nil)

    listController.initialItem = initialItem
    listController.initialItemPlaybackOffset = initialItemPlaybackOffset
    listController.delegate = self
    detailController.delegate = self

    modalPresentationStyle = .fullScreen
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    PlaylistCoordinator.shared.isPlaylistControllerPresented = false

    // Store the last played item's time-offset
    if let item = PlaylistCoordinator.shared.currentPlaylistItem {
      updateLastPlayedItem(item: item)
    }

    // Stop picture in picture
    player.pictureInPictureController?.delegate = nil
    player.pictureInPictureController?.stopPictureInPicture()

    // Simulator cannot "detect" if Car-Play is enabled, therefore we need to STOP playback
    // When this controller deallocates. The user can still manually resume playback in CarPlay.
    if !PlaylistCoordinator.shared.isCarPlayAvailable {
      // Stop media playback
      stop(playerView)
      PlaylistCoordinator.shared.currentPlaylistItem = nil
      PlaylistCoordinator.shared.currentlyPlayingItemIndex = -1

      // Destroy folder observers
      folderObserver = nil
      PlaylistManager.shared.currentFolder = nil
    }

    // Cancel all loading.
    listController.stopLoadingSharedPlaylist()
    PlaylistManager.shared.playbackTask = nil
    PlaylistCoordinator.shared.playlistController = nil
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
      $0.primaryEdge =
        PlayListSide(rawValue: Preferences.Playlist.listViewSide.value) == .left
        ? .leading : .trailing
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
      let item = PlaylistItem.getItem(uuid: initialItem.tagId)
    {
      listController.loadingState = .fullyLoaded
      PlaylistManager.shared.currentFolder = item.playlistFolder
    } else if let url = Preferences.Playlist.lastPlayedItemUrl.value,
      let item = PlaylistItem.getItems(pageSrc: url).first
    {
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

    NotificationCenter.default.do {
      $0.addObserver(
        self,
        selector: #selector(saveLastPlayedPosition),
        name: UIApplication.willResignActiveNotification,
        object: nil
      )
      $0.addObserver(
        self,
        selector: #selector(saveLastPlayedPosition),
        name: UIApplication.willTerminateNotification,
        object: nil
      )
    }
  }

  @objc
  private func saveLastPlayedPosition() {
    if let item = PlaylistCoordinator.shared.currentPlaylistItem {
      updateLastPlayedItem(item: item)
    }
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
        self.folderController.navigationController?.pushViewController(
          self.listController,
          animated: true
        )
      }
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)

    folderObserver = nil
  }

  override func viewWillTransition(
    to size: CGSize,
    with coordinator: UIViewControllerTransitionCoordinator
  ) {
    super.viewWillTransition(to: size, with: coordinator)

    updateLayoutForOrientationChange()
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    return .lightContent
  }

  private func updateLayoutForOrientationMode() {
    detailController.navigationController?.setNavigationBarHidden(
      splitController.isCollapsed || traitCollection.horizontalSizeClass == .regular,
      animated: false
    )

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
        splitController.preferredDisplayMode =
          PlaylistManager.shared.currentFolder?.isPersistent == true
          ? .oneOverSecondary : .secondaryOnly
      } else {
        splitController.preferredDisplayMode = .oneOverSecondary
      }
    }
  }

  private func updatePlayerUI() {
    // Update play/pause button
    if isPlaying {
      playerView.controlsView.playPauseButton.setImage(
        UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!,
        for: .normal
      )
    } else {
      playerView.controlsView.playPauseButton.setImage(
        UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!,
        for: .normal
      )
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
      playerView.controlsView.repeatButton.setImage(
        UIImage(named: "playlist_repeat", in: .module, compatibleWith: nil)!,
        for: .normal
      )
    case .repeatOne:
      playerView.controlsView.repeatButton.setImage(
        UIImage(named: "playlist_repeat_one", in: .module, compatibleWith: nil)!,
        for: .normal
      )
    case .repeatAll:
      playerView.controlsView.repeatButton.setImage(
        UIImage(named: "playlist_repeat_all", in: .module, compatibleWith: nil)!,
        for: .normal
      )
    }

    if let item = PlaylistCoordinator.shared.currentPlaylistItem {
      playerView.setVideoInfo(
        videoDomain: item.pageSrc,
        videoTitle: item.pageTitle,
        isPrivateBrowsing: isPrivateBrowsing
      )
    } else {
      playerView.resetVideoInfo()
    }
  }

  private func observePlayerStates() {
    player.publisher(for: .play).sink { [weak self] event in
      guard let self = self else { return }
      self.playerView.controlsView.playPauseButton.setImage(
        UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!,
        for: .normal
      )

      if !PlaylistCoordinator.shared.isCarPlayAvailable {
        MPNowPlayingInfoCenter.default().playbackState = .playing
        NowPlayingInfo.updateNowPlayingInfo(event.mediaPlayer)
      } else if let item = PlaylistCoordinator.shared.currentPlaylistItem {
        self.playerView.setVideoInfo(
          videoDomain: item.pageSrc,
          videoTitle: item.pageTitle,
          isPrivateBrowsing: self.isPrivateBrowsing
        )
      }

      self.listController.highlightActiveItem()
    }.store(in: &playerStateObservers)

    player.publisher(for: .pause).sink { [weak self] event in
      self?.playerView.controlsView.playPauseButton.setImage(
        UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!,
        for: .normal
      )

      if !PlaylistCoordinator.shared.isCarPlayAvailable {
        MPNowPlayingInfoCenter.default().playbackState = .paused
        NowPlayingInfo.updateNowPlayingInfo(event.mediaPlayer)
      }
    }.store(in: &playerStateObservers)

    player.publisher(for: .stop).sink { [weak self] _ in
      guard let self = self else { return }
      self.playerView.controlsView.playPauseButton.setImage(
        UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!,
        for: .normal
      )
      self.playerView.resetVideoInfo()
      NowPlayingInfo.clearNowPlayingInfo()

      PlaylistCoordinator.shared.currentlyPlayingItemIndex = -1
      PlaylistCoordinator.shared.currentPlaylistItem = nil

      // Cancel all loading.
      PlaylistManager.shared.playbackTask = nil
    }.store(in: &playerStateObservers)

    player.publisher(for: .changePlaybackRate).sink { [weak self] event in
      guard let self = self else { return }

      let playbackRate = event.mediaPlayer.rate
      let button = self.playerView.controlsView.playbackRateButton

      if playbackRate <= 1.0 {
        button.setTitle("1x", for: .normal)
      } else if playbackRate == 1.5 {
        button.setTitle("1.5x", for: .normal)
      } else {
        button.setTitle("2.0x", for: .normal)
      }

      if !PlaylistCoordinator.shared.isCarPlayAvailable {
        MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackRate] =
          event.mediaPlayer.rate
      }
    }.store(in: &playerStateObservers)

    player.publisher(for: .changeRepeatMode).sink { [weak self] _ in
      guard let self = self else { return }
      switch self.repeatMode {
      case .none:
        self.playerView.controlsView.repeatButton.setImage(
          UIImage(named: "playlist_repeat", in: .module, compatibleWith: nil)!,
          for: .normal
        )
      case .repeatOne:
        self.playerView.controlsView.repeatButton.setImage(
          UIImage(named: "playlist_repeat_one", in: .module, compatibleWith: nil)!,
          for: .normal
        )
      case .repeatAll:
        self.playerView.controlsView.repeatButton.setImage(
          UIImage(named: "playlist_repeat_all", in: .module, compatibleWith: nil)!,
          for: .normal
        )
      }
    }.store(in: &playerStateObservers)

    player.publisher(for: .previousTrack).sink { [weak self] _ in
      guard let self = self, !PlaylistCoordinator.shared.isCarPlayAvailable else { return }
      self.onPreviousTrack(self.playerView, isUserInitiated: true)
    }.store(in: &playerStateObservers)

    player.publisher(for: .nextTrack).sink { [weak self] _ in
      guard let self = self, !PlaylistCoordinator.shared.isCarPlayAvailable else { return }
      self.onNextTrack(self.playerView, isUserInitiated: true)
    }.store(in: &playerStateObservers)

    player.publisher(for: .finishedPlaying).sink { [weak self] event in
      guard let self = self,
        let currentItem = event.mediaPlayer.currentItem
      else { return }

      // When CarPlay is available, do NOT pause or handle `nextTrack`
      // CarPlay will do all of that. So, just update the UI only.
      if PlaylistCoordinator.shared.isCarPlayAvailable {
        self.playerView.controlsView.playPauseButton.isEnabled = false
        self.playerView.controlsView.playPauseButton.setImage(
          UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!,
          for: .normal
        )

        let endTime = CMTimeConvertScale(
          currentItem.asset.duration,
          timescale: event.mediaPlayer.currentTime.timescale,
          method: .roundHalfAwayFromZero
        )

        self.playerView.controlsView.trackBar.setTimeRange(
          currentTime: currentItem.currentTime(),
          endTime: endTime
        )
        event.mediaPlayer.seek(to: .zero)

        self.playerView.controlsView.playPauseButton.isEnabled = true
        self.playerView.controlsView.playPauseButton.setImage(
          UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!,
          for: .normal
        )

        self.playerView.toggleOverlays(showOverlay: true)
      } else {
        var nowPlayingInfo = MPNowPlayingInfoCenter.default().nowPlayingInfo
        nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackProgress] = 0.0
        nowPlayingInfo?[MPNowPlayingInfoPropertyElapsedPlaybackTime] = 0.0
        nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackRate] = 0.0
        MPNowPlayingInfoCenter.default().nowPlayingInfo = nowPlayingInfo

        self.playerView.controlsView.playPauseButton.isEnabled = false
        self.playerView.controlsView.playPauseButton.setImage(
          UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!,
          for: .normal
        )

        event.mediaPlayer.pause()

        let endTime = CMTimeConvertScale(
          currentItem.asset.duration,
          timescale: event.mediaPlayer.currentTime.timescale,
          method: .roundHalfAwayFromZero
        )

        self.playerView.controlsView.trackBar.setTimeRange(
          currentTime: currentItem.currentTime(),
          endTime: endTime
        )
        event.mediaPlayer.seek(to: .zero)

        if let item = PlaylistCoordinator.shared.currentPlaylistItem {
          self.updateLastPlayedItem(item: item)
        }

        self.playerView.controlsView.playPauseButton.isEnabled = true
        self.playerView.controlsView.playPauseButton.setImage(
          UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!,
          for: .normal
        )

        self.playerView.toggleOverlays(showOverlay: true)
        self.onNextTrack(self.playerView, isUserInitiated: false)
      }
    }.store(in: &playerStateObservers)

    player.publisher(for: .periodicPlayTimeChanged).sink { [weak self] event in
      guard let self = self, let currentItem = event.mediaPlayer.currentItem else { return }

      let endTime = CMTimeConvertScale(
        currentItem.asset.duration,
        timescale: event.mediaPlayer.currentTime.timescale,
        method: .roundHalfAwayFromZero
      )

      if CMTimeCompare(endTime, .zero) != 0 && endTime.value > 0 {
        self.playerView.controlsView.trackBar.setTimeRange(
          currentTime: event.mediaPlayer.currentTime,
          endTime: endTime
        )
      }
    }.store(in: &playerStateObservers)

    self.playerView.infoView.pictureInPictureButton.isEnabled =
      AVPictureInPictureController.isPictureInPictureSupported()
    player.publisher(for: .pictureInPictureStatusChanged).sink { [weak self] event in
      guard let self = self else { return }

      self.playerView.infoView.pictureInPictureButton.isEnabled =
        event.mediaPlayer.pictureInPictureController?.isPictureInPicturePossible == true
    }.store(in: &playerStateObservers)
  }

  private func observeFolderStates() {
    folderController.onFolderSelected = { folder in
      PlaylistManager.shared.currentFolder = folder
    }
  }
}

// MARK: - UIAdaptivePresentationControllerDelegate

extension PlaylistViewController: UIAdaptivePresentationControllerDelegate {
  func adaptivePresentationStyle(
    for controller: UIPresentationController,
    traitCollection: UITraitCollection
  ) -> UIModalPresentationStyle {
    return .fullScreen
  }
}

// MARK: - UISplitViewControllerDelegate

extension PlaylistViewController: UISplitViewControllerDelegate {

  func splitViewControllerSupportedInterfaceOrientations(
    _ splitViewController: UISplitViewController
  ) -> UIInterfaceOrientationMask {
    return .allButUpsideDown
  }

  func splitViewController(
    _ splitViewController: UISplitViewController,
    collapseSecondary secondaryViewController: UIViewController,
    onto primaryViewController: UIViewController
  ) -> Bool {

    // On iPhone, always display the iPhone layout (collapsed) no matter what.
    // On iPad, we need to update both the list controller's layout (collapsed) and the detail controller's layout (collapsed).
    listController.updateLayoutForMode(.phone)
    detailController.setVideoPlayer(nil)
    detailController.updateLayoutForMode(.phone)
    return true
  }

  func splitViewController(
    _ splitViewController: UISplitViewController,
    separateSecondaryFrom primaryViewController: UIViewController
  ) -> UIViewController? {

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
    NowPlayingInfo.clearNowPlayingInfo()

    PlaylistCoordinator.shared.currentlyPlayingItemIndex = -1
    PlaylistCoordinator.shared.currentPlaylistItem = nil
    playerView.resetVideoInfo()
    stop(playerView)

    // Cancel all loading.
    PlaylistManager.shared.playbackTask?.cancel()
    PlaylistManager.shared.playbackTask = nil
  }

  func deleteItem(itemId: String, at index: Int) {
    guard let item = PlaylistItem.getItem(uuid: itemId) else {
      return
    }

    Task { @MainActor in
      await PlaylistManager.shared.delete(item: PlaylistInfo(item: item))

      if PlaylistCoordinator.shared.currentlyPlayingItemIndex == index
        || PlaylistManager.shared.numberOfAssets == 0
      {
        stopPlaying()
      }
    }
  }

  func updateLastPlayedItem(item: PlaylistInfo) {
    guard let playTime = player.currentItem?.currentTime() else {
      return
    }

    PlaylistManager.shared.updateLastPlayed(item: item, playTime: playTime.seconds)
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
    if PlaylistCoordinator.shared.currentlyPlayingItemIndex <= 0 {
      return
    }

    saveLastPlayedPosition()
    let index = PlaylistCoordinator.shared.currentlyPlayingItemIndex - 1
    if index < PlaylistManager.shared.numberOfAssets {
      let indexPath = IndexPath(row: index, section: 0)
      listController.prepareToPlayItem(at: indexPath) { [weak self] item in
        guard let self = self,
          let item = item
        else {

          self?.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
          return
        }

        PlaylistCoordinator.shared.currentlyPlayingItemIndex = indexPath.row
        PlaylistCoordinator.shared.currentPlaylistItem = item

        PlaylistManager.shared.playbackTask = Task { @MainActor in
          do {
            let item = try await self.playItem(item: item)
            PlaylistCoordinator.shared.currentlyPlayingItemIndex = index
            PlaylistCoordinator.shared.currentPlaylistItem = item
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            self.updateLastPlayedItem(item: item)
          } catch {
            Logger.module.error("Playlist Error Playing Item: \(error)")
            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            self.displayLoadingResourceError()
          }
        }
      }
    }
  }

  func onNextTrack(_ videoView: VideoView, isUserInitiated: Bool) {
    let assetCount = PlaylistManager.shared.numberOfAssets
    let isAtEnd = PlaylistCoordinator.shared.currentlyPlayingItemIndex >= assetCount - 1
    var index = PlaylistCoordinator.shared.currentlyPlayingItemIndex
    saveLastPlayedPosition()

    switch repeatMode {
    case .none:
      if isAtEnd {
        player.pictureInPictureController?.delegate = nil
        player.pictureInPictureController?.stopPictureInPicture()
        player.stop()
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

        PlaylistCoordinator.shared.currentlyPlayingItemIndex = indexPath.row
        PlaylistCoordinator.shared.currentPlaylistItem = item

        PlaylistManager.shared.playbackTask = Task { @MainActor in
          do {
            let item = try await self.playItem(item: item)
            PlaylistCoordinator.shared.currentPlaylistItem = item

            self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
            PlaylistCoordinator.shared.currentlyPlayingItemIndex = index
            PlaylistCoordinator.shared.currentPlaylistItem = item
            self.updateLastPlayedItem(item: item)
          } catch {
            PlaylistCoordinator.shared.currentPlaylistItem = nil
            PlaylistCoordinator.shared.currentlyPlayingItemIndex = -1
            Logger.module.error("Playlist Error Playing Item: \(error)")

            if isUserInitiated || self.repeatMode == .repeatOne || assetCount <= 1 {
              self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: true)
              self.displayExpiredResourceError(item: item)
            } else {
              DispatchQueue.main.async {
                self.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
                PlaylistCoordinator.shared.currentlyPlayingItemIndex = index
                self.onNextTrack(videoView, isUserInitiated: isUserInitiated)
              }
            }
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
      playerView.controlsView.playPauseButton.setImage(
        UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!,
        for: .normal
      )
      playerView.toggleOverlays(showOverlay: false)
      playerView.isOverlayDisplayed = false

      player.play()
    }
  }

  func pause(_ videoView: VideoView) {
    if isPlaying {
      playerView.controlsView.playPauseButton.setImage(
        UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!,
        for: .normal
      )
      playerView.toggleOverlays(showOverlay: true)
      playerView.isOverlayDisplayed = true

      player.pause()
    } else {
      playerView.toggleOverlays(showOverlay: playerView.isOverlayDisplayed)
    }
  }

  func stop(_ videoView: VideoView) {
    playerView.controlsView.playPauseButton.setImage(
      UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!,
      for: .normal
    )
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
      let seekTime = CMTimeMakeWithSeconds(
        Float64(
          CGFloat(relativeOffset) * CGFloat(currentItem.asset.duration.value)
            / CGFloat(currentItem.asset.duration.timescale)
        ),
        preferredTimescale: currentItem.currentTime().timescale
      )
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

  private func clear() {
    NowPlayingInfo.clearNowPlayingInfo()
    player.clear()

    PlaylistManager.shared.playbackTask?.cancel()
    PlaylistManager.shared.playbackTask = nil
  }

  func load(_ videoView: VideoView, url: URL, autoPlayEnabled: Bool) async throws {
    try await load(
      videoView,
      asset: AVURLAsset(url: url, options: AVAsset.defaultOptions),
      autoPlayEnabled: autoPlayEnabled
    )
  }

  /// - throws: MediaPlaybackError
  func load(
    _ videoView: VideoView,
    asset: AVURLAsset,
    autoPlayEnabled: Bool
  ) async throws {
    // Task will be nil if the playback has stopped, but not paused
    // If it is paused, and we're loading another track, don't bother clearing the player
    // as this will break PIP
    if PlaylistManager.shared.playbackTask == nil {
      self.clear()
    }

    let isNewItem = try await player.load(asset: asset)

    guard let item = player.currentItem else {
      throw MediaPlaybackError.cancelled
    }

    // We are playing the same item again..
    if !isNewItem {
      pause(videoView)
      seek(videoView, relativeOffset: 0.0)  // Restart playback
      play(videoView)
      return
    }

    // Live media item
    let isPlayingLiveMedia = self.player.isLiveMedia
    self.playerView.setMediaIsLive(isPlayingLiveMedia)

    // Track-bar
    let endTime = CMTimeConvertScale(
      item.asset.duration,
      timescale: self.player.currentTime.timescale,
      method: .roundHalfAwayFromZero
    )
    self.playerView.controlsView.trackBar.setTimeRange(
      currentTime: item.currentTime(),
      endTime: endTime
    )

    // Successfully loaded
    if autoPlayEnabled {
      self.play(videoView)  // Play the new item
    }
  }

  func playItem(item: PlaylistInfo) async throws -> PlaylistInfo {
    // This MUST be checked.
    // The user must not be able to alter a player that isn't visible from any UI!
    // This is because, if car-play's interface is attached, the player can only be
    // controller through this UI so long as it is attached to it.
    // If it isn't attached, the player can only be controlled through the car-play interface.
    guard player.isAttachedToDisplay else {
      throw PlaylistMediaStreamer.PlaybackError.cancelled
    }

    // If the item is cached, load it from the cache and play it.
    let cacheState = PlaylistManager.shared.state(for: item.tagId)
    if cacheState != .invalid {
      if let index = PlaylistManager.shared.index(of: item.tagId),
        let asset = PlaylistManager.shared.assetAtIndexSynchronous(index)
      {

        do {
          try await load(playerView, asset: asset, autoPlayEnabled: listController.autoPlayEnabled)
          NowPlayingInfo.clearNowPlayingInfo()
          self.playerView.setVideoInfo(
            videoDomain: item.pageSrc,
            videoTitle: item.pageTitle,
            isPrivateBrowsing: self.isPrivateBrowsing
          )
          NowPlayingInfo.setNowPlayingInfo(item, withPlayer: self.player)
        } catch {
          NowPlayingInfo.clearNowPlayingInfo()
          throw error
        }
      } else {
        NowPlayingInfo.clearNowPlayingInfo()
        throw PlaylistMediaStreamer.PlaybackError.expired
      }
      return item
    }

    // The item is not cached so we should attempt to stream it
    return try await streamItem(item: item)
  }

  func streamItem(item: PlaylistInfo) async throws -> PlaylistInfo {
    var item = item

    do {
      item = try await mediaStreamer.loadMediaStreamingAsset(item)
    } catch {
      NowPlayingInfo.clearNowPlayingInfo()
      throw error
    }

    // Item can be streamed
    guard let url = URL(string: item.src) else {
      NowPlayingInfo.clearNowPlayingInfo()
      throw PlaylistMediaStreamer.PlaybackError.expired
    }

    // Attempt to play the stream
    do {
      try await load(
        self.playerView,
        url: url,
        autoPlayEnabled: self.listController.autoPlayEnabled
      )
    } catch {
      NowPlayingInfo.clearNowPlayingInfo()
      throw PlaylistMediaStreamer.PlaybackError.cannotLoadMedia
    }

    playerView.setVideoInfo(
      videoDomain: item.pageSrc,
      videoTitle: item.pageTitle,
      isPrivateBrowsing: isPrivateBrowsing
    )

    NowPlayingInfo.setNowPlayingInfo(item, withPlayer: self.player)
    return item
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
