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

import BraveShared
import Shared
import SDWebImage
import CoreData
import Data

private let log = Logger.browserLogger

// MARK: PlaylistViewControllerDelegate
protocol PlaylistViewControllerDelegate: AnyObject {
    func attachPlayerView()
    func detachPlayerView()
    func onSidePanelStateChanged()
    func onFullscreen()
    func onExitFullscreen()
    func playItem(item: PlaylistInfo, completion: ((PlaylistMediaStreamer.PlaybackError) -> Void)?)
    func deleteItem(item: PlaylistInfo, at index: Int)
    func updateLastPlayedItem(item: PlaylistInfo)
    func displayLoadingResourceError()
    func displayExpiredResourceError(item: PlaylistInfo)
    
    var isPlaying: Bool { get }
    var currentPlaylistItem: AVPlayerItem? { get }
    var currentPlaylistAsset: AVAsset? { get }
}

// MARK: PlaylistViewController

class PlaylistViewController: UIViewController {
    
    // MARK: Properties

    private let player: MediaPlayer
    private let playerView = VideoView()
    private lazy var mediaStreamer = PlaylistMediaStreamer(playerView: playerView)
    
    private let splitController = UISplitViewController()
    private lazy var listController = PlaylistListViewController(playerView: playerView)
    private let detailController = PlaylistDetailViewController()
    
    private var playerStateObservers = Set<AnyCancellable>()
    private var assetStateObservers = Set<AnyCancellable>()
    private var assetLoadingStateObservers = Set<AnyCancellable>()
    
    init(mediaPlayer: MediaPlayer, initialItem: PlaylistInfo?, initialItemPlaybackOffset: Double) {
        self.player = mediaPlayer
        super.init(nibName: nil, bundle: nil)
        
        listController.initialItem = initialItem
        listController.initialItemPlaybackOffset = initialItemPlaybackOffset
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    deinit {
        // Store the last played item's time-offset
        if let playTime = player.currentItem?.currentTime(),
           Preferences.Playlist.playbackLeftOff.value {
            Preferences.Playlist.lastPlayedItemTime.value = playTime.seconds
        } else {
            Preferences.Playlist.lastPlayedItemTime.value = 0.0
        }
        
        // Stop picture in picture
        player.pictureInPictureController?.delegate = nil
        player.pictureInPictureController?.stopPictureInPicture()
        
        // Simulator cannot "detect" if Car-Play is enabled, therefore we need to STOP playback
        // When this controller deallocates. The user can still manually resume playback in CarPlay.
        #if targetEnvironment(simulator)
        // Stop media playback
        stop(playerView)
        PlaylistCarplayManager.shared.currentPlaylistItem = nil
        #endif
        
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
        listController.delegate = self
        
        // Layout
        splitController.do {
            $0.viewControllers = [SettingsNavigationController(rootViewController: listController),
                                  SettingsNavigationController(rootViewController: detailController)]
            $0.delegate = self
            $0.primaryEdge = PlayListSide(rawValue: Preferences.Playlist.listViewSide.value) == .left ? .leading : .trailing
            $0.presentsWithGesture = false
            $0.maximumPrimaryColumnWidth = 400
            $0.minimumPrimaryColumnWidth = 400
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
    
    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)
        
        updateLayoutForOrientationChange()
    }
    
    override var preferredStatusBarStyle: UIStatusBarStyle {
        return .lightContent
    }
    
    private func updateLayoutForOrientationChange() {
        if playerView.isFullscreen {
            splitController.preferredDisplayMode = .secondaryOnly
        } else {
            if UIDevice.current.orientation.isLandscape {
                splitController.preferredDisplayMode = .secondaryOnly
            } else {
                splitController.preferredDisplayMode = .primaryOverlay
            }
        }
    }
    
    private func updatePlayerUI() {
        // Update play/pause button
        if isPlaying {
            playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_pause"), for: .normal)
        } else {
            playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)
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
            playerView.controlsView.repeatButton.setImage(#imageLiteral(resourceName: "playlist_repeat"), for: .normal)
        case .repeatOne:
            playerView.controlsView.repeatButton.setImage(#imageLiteral(resourceName: "playlist_repeat_one"), for: .normal)
        case .repeatAll:
            playerView.controlsView.repeatButton.setImage(#imageLiteral(resourceName: "playlist_repeat_all"), for: .normal)
        }
        
        if let item = PlaylistCarplayManager.shared.currentPlaylistItem {
            playerView.setVideoInfo(videoDomain: item.pageSrc, videoTitle: item.pageTitle)
        } else {
            playerView.resetVideoInfo()
        }
    }
    
    private func observePlayerStates() {
        player.publisher(for: .play).sink { [weak self] event in
            self?.playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_pause"), for: .normal)
            
            if !PlaylistCarplayManager.shared.isCarPlayAvailable {
                MPNowPlayingInfoCenter.default().playbackState = .playing
                PlaylistMediaStreamer.updateNowPlayingInfo(event.mediaPlayer)
            }
        }.store(in: &playerStateObservers)
        
        player.publisher(for: .pause).sink { [weak self] event in
            self?.playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)
            
            if !PlaylistCarplayManager.shared.isCarPlayAvailable {
                MPNowPlayingInfoCenter.default().playbackState = .paused
                PlaylistMediaStreamer.updateNowPlayingInfo(event.mediaPlayer)
            }
        }.store(in: &playerStateObservers)
        
        player.publisher(for: .stop).sink { [weak self] _ in
            guard let self = self else { return }
            self.playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)
            self.playerView.resetVideoInfo()
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
                button.setTitle("2x", for: .normal)
            }
            
            if !PlaylistCarplayManager.shared.isCarPlayAvailable {
                MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackRate] = event.mediaPlayer.rate
            }
        }.store(in: &playerStateObservers)
        
        player.publisher(for: .changeRepeatMode).sink { [weak self] _ in
            guard let self = self else { return }
            switch self.repeatMode {
            case .none:
                self.playerView.controlsView.repeatButton.setImage(#imageLiteral(resourceName: "playlist_repeat"), for: .normal)
            case .repeatOne:
                self.playerView.controlsView.repeatButton.setImage(#imageLiteral(resourceName: "playlist_repeat_one"), for: .normal)
            case .repeatAll:
                self.playerView.controlsView.repeatButton.setImage(#imageLiteral(resourceName: "playlist_repeat_all"), for: .normal)
            }
        }.store(in: &playerStateObservers)
        
        player.publisher(for: .finishedPlaying).sink { [weak self] event in
            guard let self = self ,
                  let currentItem = event.mediaPlayer.currentItem else { return }
            
            // When CarPlay is available, do NOT pause or handle `nextTrack`
            // CarPlay will do all of that. So, just update the UI only.
            if PlaylistCarplayManager.shared.isCarPlayAvailable {
                self.playerView.controlsView.playPauseButton.isEnabled = false
                self.playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_pause"), for: .normal)

                let endTime = CMTimeConvertScale(currentItem.asset.duration, timescale: event.mediaPlayer.currentTime.timescale, method: .roundHalfAwayFromZero)

                self.playerView.controlsView.trackBar.setTimeRange(currentTime: currentItem.currentTime(), endTime: endTime)
                event.mediaPlayer.seek(to: .zero)
                
                self.playerView.controlsView.playPauseButton.isEnabled = true
                self.playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)

                self.playerView.toggleOverlays(showOverlay: true)
            } else {
                var nowPlayingInfo = MPNowPlayingInfoCenter.default().nowPlayingInfo
                nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackProgress] = 0.0
                nowPlayingInfo?[MPNowPlayingInfoPropertyElapsedPlaybackTime] = 0.0
                nowPlayingInfo?[MPNowPlayingInfoPropertyPlaybackRate] = 0.0
                MPNowPlayingInfoCenter.default().nowPlayingInfo = nowPlayingInfo
                
                self.playerView.controlsView.playPauseButton.isEnabled = false
                self.playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_pause"), for: .normal)

                event.mediaPlayer.pause()

                let endTime = CMTimeConvertScale(currentItem.asset.duration, timescale: event.mediaPlayer.currentTime.timescale, method: .roundHalfAwayFromZero)

                self.playerView.controlsView.trackBar.setTimeRange(currentTime: currentItem.currentTime(), endTime: endTime)
                event.mediaPlayer.seek(to: .zero)
                
                self.playerView.controlsView.playPauseButton.isEnabled = true
                self.playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)

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
        if !UIDevice.isIpad || splitViewController?.isCollapsed == true {
            listController.onFullscreen()
        } else {
            detailController.onFullscreen()
        }
    }
    
    func onExitFullscreen() {
        listController.onExitFullscreen()
    }
    
    func deleteItem(item: PlaylistInfo, at index: Int) {
        PlaylistManager.shared.delete(item: item)
        
        if PlaylistCarplayManager.shared.currentlyPlayingItemIndex == index {
            PlaylistMediaStreamer.clearNowPlayingInfo()
            
            PlaylistCarplayManager.shared.currentlyPlayingItemIndex = -1
            playerView.resetVideoInfo()
            stop(playerView)
            
            // Cancel all loading.
            assetLoadingStateObservers.removeAll()
            assetStateObservers.removeAll()
        }
    }
    
    func updateLastPlayedItem(item: PlaylistInfo) {
        Preferences.Playlist.lastPlayedItemUrl.value = item.pageSrc
        
        if let playTime = player.currentItem?.currentTime(),
           Preferences.Playlist.playbackLeftOff.value {
            Preferences.Playlist.lastPlayedItemTime.value = playTime.seconds
        } else {
            Preferences.Playlist.lastPlayedItemTime.value = 0.0
        }
    }
    
    func displayLoadingResourceError() {
        let isPrimaryDisplayMode = splitController.preferredDisplayMode == .primaryOverlay
        if isPrimaryDisplayMode {
            listController.displayLoadingResourceError()
        } else {
            detailController.displayLoadingResourceError()
        }
    }
    
    func displayExpiredResourceError(item: PlaylistInfo) {
        let isPrimaryDisplayMode = splitController.preferredDisplayMode == .primaryOverlay
        if isPrimaryDisplayMode {
            listController.displayExpiredResourceError(item: item)
        } else {
            detailController.displayExpiredResourceError(item: item)
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
                      let item = item else {
                    
                    self?.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
                    return
                }
                
                PlaylistCarplayManager.shared.currentlyPlayingItemIndex = indexPath.row
                self.playItem(item: item) { [weak self] error in
                    PlaylistCarplayManager.shared.currentPlaylistItem = nil
                    
                    guard let self = self else { return }
                    
                    switch error {
                    case .other(let err):
                        log.error(err)
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
                        log.debug("User Cancelled Playlist Playback")
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
            listController.prepareToPlayItem(at: indexPath) { [weak self] item in
                guard let self = self,
                      let item = item else {
                    
                    self?.listController.commitPlayerItemTransaction(at: indexPath, isExpired: false)
                    return
                }
                
                PlaylistCarplayManager.shared.currentlyPlayingItemIndex = indexPath.row
                self.playItem(item: item) { [weak self] error in
                    PlaylistCarplayManager.shared.currentPlaylistItem = nil
                    guard let self = self else { return }
                    
                    switch error {
                    case .other(let err):
                        log.error(err)
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
                        log.debug("User Cancelled Playlist Playback")
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
                if #available(iOS 14.0, *) {
                    pictureInPictureController.requiresLinearPlayback = false
                }

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
            playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_pause"), for: .normal)
            playerView.toggleOverlays(showOverlay: false)
            playerView.isOverlayDisplayed = false
            
            player.play()
        }
    }
    
    func pause(_ videoView: VideoView) {
        if isPlaying {
            playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)
            playerView.toggleOverlays(showOverlay: true)
            playerView.isOverlayDisplayed = true
            
            player.pause()
        } else {
            playerView.toggleOverlays(showOverlay: playerView.isOverlayDisplayed)
        }
    }
    
    func stop(_ videoView: VideoView) {
        playerView.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)
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
    
    func load(_ videoView: VideoView, url: URL, autoPlayEnabled: Bool) -> AnyPublisher<Void, Error> {
        load(videoView, asset: AVURLAsset(url: url), autoPlayEnabled: autoPlayEnabled)
    }
    
    func load(_ videoView: VideoView, asset: AVURLAsset, autoPlayEnabled: Bool) -> AnyPublisher<Void, Error> {
        assetLoadingStateObservers.removeAll()
        player.stop()
        
        return Future { [weak self] resolver in
            guard let self = self else {
                resolver(.failure("User Cancelled Playback"))
                return
            }
            
            self.player.load(asset: asset)
            .receive(on: RunLoop.main)
            .sink(receiveCompletion: { status in
                switch status {
                case .failure(let error):
                    resolver(.failure(error))
                case .finished:
                    break
                }
            }, receiveValue: { [weak self] isNewItem in
                guard let self = self else {
                    resolver(.failure("User Cancelled Playback"))
                    return
                }
                
                guard let item = self.player.currentItem else {
                    resolver(.failure("Couldn't load playlist item"))
                    return
                }
                
                // We are playing the same item again..
                if !isNewItem {
                    self.pause(videoView)
                    self.seek(videoView, relativeOffset: 0.0) // Restart playback
                    self.play(videoView)
                    resolver(.success(Void()))
                    return
                }
                
                // Live media item
                let isPlayingLiveMedia = self.player.isLiveMedia
                self.playerView.controlsView.trackBar.isUserInteractionEnabled = !isPlayingLiveMedia
                self.playerView.controlsView.skipBackButton.isEnabled = !isPlayingLiveMedia
                self.playerView.controlsView.skipForwardButton.isEnabled = !isPlayingLiveMedia
                
                // Track-bar
                let endTime = CMTimeConvertScale(item.asset.duration, timescale: self.player.currentTime.timescale, method: .roundHalfAwayFromZero)
                self.playerView.controlsView.trackBar.setTimeRange(currentTime: item.currentTime(), endTime: endTime)
                
                // Successfully loaded
                resolver(.success(Void()))
                
                if autoPlayEnabled {
                    self.play(videoView) // Play the new item
                }
            }).store(in: &self.assetLoadingStateObservers)
        }.eraseToAnyPublisher()
    }
    
    func playItem(item: PlaylistInfo, completion: ((PlaylistMediaStreamer.PlaybackError) -> Void)?) {
        assetLoadingStateObservers.removeAll()
        assetStateObservers.removeAll()
        
        // This MUST be checked.
        // The user must not be able to alter a player that isn't visible from any UI!
        // This is because, if car-play is interface is attached, the player can only be
        // controller through this UI so long as it is attached to it.
        // If it isn't attached, the player can only be controlled through the car-play interface.
        guard player.isAttachedToDisplay else {
            completion?(.cancelled)
            return
        }

        // If the item is cached, load it from the cache and play it.
        let cacheState = PlaylistManager.shared.state(for: item.pageSrc)
        if cacheState != .invalid {
            if let index = PlaylistManager.shared.index(of: item.pageSrc),
               let asset = PlaylistManager.shared.assetAtIndex(index) {
                load(playerView, asset: asset, autoPlayEnabled: listController.autoPlayEnabled)
                .handleEvents(receiveCancel: {
                    PlaylistMediaStreamer.clearNowPlayingInfo()
                    completion?(.cancelled)
                })
                .sink(receiveCompletion: { status in
                    switch status {
                    case .failure(let error):
                        PlaylistMediaStreamer.clearNowPlayingInfo()
                        completion?(.other(error))
                    case .finished:
                        break
                    }
                }, receiveValue: { [weak self] _ in
                    guard let self = self else {
                        PlaylistMediaStreamer.clearNowPlayingInfo()
                        completion?(.cancelled)
                        return
                    }
                    
                    PlaylistMediaStreamer.clearNowPlayingInfo()
                    self.playerView.setVideoInfo(videoDomain: item.pageSrc,
                                                 videoTitle: item.pageTitle)
                    PlaylistMediaStreamer.setNowPlayingInfo(item, withPlayer: self.player)
                    completion?(.none)
                }).store(in: &assetLoadingStateObservers)
            } else {
                completion?(.expired)
            }
            return
        }
        
        // The item is not cached so we should attempt to stream it
        streamItem(item: item, completion: completion)
    }
    
    func streamItem(item: PlaylistInfo, completion: ((PlaylistMediaStreamer.PlaybackError) -> Void)?) {
        mediaStreamer.loadMediaStreamingAsset(item)
        .handleEvents(receiveCancel: {
            PlaylistMediaStreamer.clearNowPlayingInfo()
            completion?(.cancelled)
        })
        .sink(receiveCompletion: { status in
            switch status {
            case .failure(let error):
                PlaylistMediaStreamer.clearNowPlayingInfo()
                completion?(error)
            case .finished:
                break
            }
        }, receiveValue: { [weak self] _ in
            guard let self = self else {
                PlaylistMediaStreamer.clearNowPlayingInfo()
                completion?(.cancelled)
                return
            }
            
            // Item can be streamed, so let's retrieve its URL from our DB
            guard let index = PlaylistManager.shared.index(of: item.pageSrc),
                  let item = PlaylistManager.shared.itemAtIndex(index) else {
                PlaylistMediaStreamer.clearNowPlayingInfo()
                completion?(.expired)
                return
            }
            
            // Attempt to play the stream
            if let url = URL(string: item.src) {
                self.load(self.playerView,
                          url: url,
                          autoPlayEnabled: self.listController.autoPlayEnabled)
                .handleEvents(receiveCancel: {
                    PlaylistMediaStreamer.clearNowPlayingInfo()
                    completion?(.cancelled)
                })
                .sink(receiveCompletion: { status in
                    switch status {
                    case .failure(let error):
                        PlaylistMediaStreamer.clearNowPlayingInfo()
                        completion?(.other(error))
                    case .finished:
                        break
                    }
                }, receiveValue: { [weak self] _ in
                    guard let self = self else {
                        PlaylistMediaStreamer.clearNowPlayingInfo()
                        completion?(.cancelled)
                        return
                    }
                    
                    self.playerView.setVideoInfo(videoDomain: item.pageSrc,
                                                 videoTitle: item.pageTitle)
                    PlaylistMediaStreamer.setNowPlayingInfo(item, withPlayer: self.player)
                    completion?(.none)
                }).store(in: &self.assetLoadingStateObservers)
                log.debug("Playing Live Video: \(self.player.isLiveMedia)")
            } else {
                PlaylistMediaStreamer.clearNowPlayingInfo()
                completion?(.expired)
            }
        }).store(in: &assetStateObservers)
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
        if let asset = player.currentItem?.asset {
            return asset.isVideoTracksAvailable()
        }
        
        // We do this because for m3u8 HLS streams,
        // tracks may not always be available and the particle effect will show even on videos..
        // It's best to assume this type of media is a video stream.
        return true
    }
}
