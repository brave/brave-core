// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import AVFoundation
import AVKit
import Combine
import MediaPlayer
import Shared

private let log = Logger.browserLogger

class MediaPlayer: NSObject {
    public enum RepeatMode: CaseIterable {
        case none
        case repeatOne
        case repeatAll
    }
    
    public enum ShuffleMode: CaseIterable {
        case none
        case items
        case collection
    }
    
    // MARK: - Public Variables
    
    private(set) public var isSeeking = false
    private(set) public var seekInterval: TimeInterval = 15.0
    private(set) public var supportedPlaybackRates = [1.0, 1.5, 2.0]
    private(set) public var pendingMediaItem: AVPlayerItem?
    private(set) public var pictureInPictureController: AVPictureInPictureController?
    private(set) var repeatState: RepeatMode = .none
    private(set) var shuffleState: ShuffleMode = .none
    private(set) var previousRate: Float = -1.0
    
    public var isPlaying: Bool {
        // It is better NOT to keep tracking of isPlaying OR rate > 0.0
        // Instead we should use the timeControlStatus because PIP and Background play
        // via control-center will modify the timeControlStatus property
        // This will keep our UI consistent with what is on the lock-screen.
        // This will also allow us to properly determine play state in
        // PlaylistMediaInfo -> init -> MPRemoteCommandCenter.shared().playCommand
        player.timeControlStatus == .playing
    }
    
    public var currentItem: AVPlayerItem? {
        player.currentItem
    }
    
    public var currentTime: CMTime {
        player.currentTime()
    }
    
    public var rate: Float {
        player.rate
    }
    
    public var isLiveMedia: Bool {
        (player.currentItem ?? pendingMediaItem)?.asset.duration.isIndefinite == true
    }
    
    public var isAttachedToDisplay: Bool {
        playerLayer.superlayer != nil
    }
    
    override init() {
        super.init()
        
        playerLayer.player = self.player
        
        // Register for notifications
        registerNotifications()
        registerControlCenterNotifications()
        registerPictureInPictureNotifications()
        
        // For now, disable shuffling
        MPRemoteCommandCenter.Command.changeShuffleModeCommand.command.isEnabled = false
        
        // Start receiving remote commands
        UIApplication.shared.beginReceivingRemoteControlEvents()
        
        // Enable our audio session
        do {
            try AVAudioSession.sharedInstance().setCategory(.playback, mode: .default)
            try AVAudioSession.sharedInstance().setActive(true, options: .notifyOthersOnDeactivation)
        } catch {
            log.error(error)
        }
    }
    
    deinit {
        // Unregister for notifications
        notificationObservers.removeAll()
        
        if let periodicTimeObserver = periodicTimeObserver {
            player.removeTimeObserver(periodicTimeObserver)
        }
        
        // Disable our audio session
        do {
            try AVAudioSession.sharedInstance().setCategory(.playback, mode: .default, policy: .default, options: [])
            try AVAudioSession.sharedInstance().setActive(false)
        } catch {
            log.error(error)
        }
        
        // Stop receiving remote commands
        UIApplication.shared.endReceivingRemoteControlEvents()
    }
    
    func load(url: URL) -> Combine.Deferred<AnyPublisher<Bool, Error>> {
        load(asset: AVURLAsset(url: url))
    }
    
    /// On success, returns a publisher with a Boolean.
    /// The boolean indicates if a NEW player item was loaded, OR if an existing item was loaded.
    /// If an existing item is loaded, you should seek to offset zero to restart playback.
    /// If a new item is loaded, you should call play to begin playback.
    /// Returns an error on failure.
    func load(asset: AVURLAsset) -> Combine.Deferred<AnyPublisher<Bool, Error>> {
        return Deferred { [weak self] in
            guard let self = self else {
                return Fail<Bool, Error>(error: "MediaPlayer Deallocated")
                        .eraseToAnyPublisher()
            }
            
            return Future { resolver in
                // If the same asset is being loaded again.
                // Just play it.
                if let currentItem = self.player.currentItem, currentItem.asset.isKind(of: AVURLAsset.self) && self.player.status == .readyToPlay {
                    if let currentAsset = currentItem.asset as? AVURLAsset, currentAsset.url.absoluteString == asset.url.absoluteString {
                        resolver(.success(false)) // Same item is playing.
                        self.pendingMediaItem = nil
                        return
                    }
                }

                let assetKeys = ["playable", "tracks", "duration"]
                self.pendingMediaItem = AVPlayerItem(asset: asset)
                asset.loadValuesAsynchronously(forKeys: assetKeys) { [weak self] in
                    guard let self = self, let item = self.pendingMediaItem else { return }
                    
                    for key in assetKeys {
                        var error: NSError?
                        let status = item.asset.statusOfValue(forKey: key, error: &error)
                        if let error = error {
                            resolver(.failure(error))
                            return
                        } else if status != .loaded {
                            resolver(.failure("Cannot Load Asset Status: \(status)"))
                            return
                        }
                    }
                    
                    DispatchQueue.main.async {
                        self.player.replaceCurrentItem(with: item)
                        self.pendingMediaItem = nil
                        resolver(.success(true)) // New Item loaded
                    }
                }
            }.eraseToAnyPublisher()
        }
    }
    
    func play() {
        if !isPlaying {
            player.play()
            player.rate = previousRate < 0.0 ? 1.0 : previousRate
            playSubscriber.send(EventNotification(mediaPlayer: self, event: .play))
        }
    }
    
    func pause() {
        if isPlaying {
            previousRate = player.rate
            player.pause()
            pauseSubscriber.send(EventNotification(mediaPlayer: self, event: .pause))
        }
    }
    
    func stop() {
        if isPlaying {
            previousRate = player.rate == 0.0 ? -1.0 : player.rate
            player.pause()
            player.replaceCurrentItem(with: nil)
            stopSubscriber.send(EventNotification(mediaPlayer: self, event: .stop))
        }
    }
    
    func seekPreviousTrack() {
        previousTrackSubscriber.send(EventNotification(mediaPlayer: self, event: .previousTrack))
    }
    
    func seekNextTrack() {
        nextTrackSubscriber.send(EventNotification(mediaPlayer: self, event: .nextTrack))
    }
    
    func seekBackwards() {
        if let currentItem = player.currentItem {
            let currentTime = currentItem.currentTime().seconds
            var seekTime = currentTime - seekInterval

            if seekTime < 0 {
                seekTime = 0
            }
            
            let absoluteTime = CMTimeMakeWithSeconds(seekTime, preferredTimescale: currentItem.currentTime().timescale)
            player.seek(to: absoluteTime, toleranceBefore: .zero, toleranceAfter: .zero)
            
            seekBackwardSubscriber.send(EventNotification(mediaPlayer: self, event: .seekBackward))
        }
    }
    
    func seekForwards() {
        if let currentItem = player.currentItem {
            let currentTime = currentItem.currentTime().seconds
            let seekTime = currentTime + seekInterval

            if seekTime < (currentItem.duration.seconds - seekInterval) {
                let absoluteTime = CMTimeMakeWithSeconds(seekTime, preferredTimescale: currentItem.currentTime().timescale)
                player.seek(to: absoluteTime, toleranceBefore: .zero, toleranceAfter: .zero)
                
                seekForwardSubscriber.send(EventNotification(mediaPlayer: self, event: .seekForward))
            }
        }
    }
    
    func seek(to time: TimeInterval) {
        if let currentItem = player.currentItem {
            var seekTime = time
            if seekTime < 0.0 {
                seekTime = 0.0
            }
            
            if seekTime >= currentItem.duration.seconds {
                seekTime = currentItem.duration.seconds
            }
            
            let absoluteTime = CMTimeMakeWithSeconds(seekTime, preferredTimescale: currentItem.currentTime().timescale)
            player.seek(to: absoluteTime, toleranceBefore: .zero, toleranceAfter: .zero)
            
            self.changePlaybackPositionSubscriber.send(EventNotification(mediaPlayer: self,
                                                                         event: .changePlaybackPosition))
        }
    }
    
    func toggleRepeatMode() {
        let command = MPRemoteCommandCenter.shared().changeRepeatModeCommand
        switch repeatState {
        case .none:
            command.currentRepeatType = .off
            self.repeatState = .repeatOne
        case .repeatOne:
            command.currentRepeatType = .one
            self.repeatState = .repeatAll
        case .repeatAll:
            command.currentRepeatType = .all
            self.repeatState = .none
        }
        
        changeRepeatModeSubscriber.send(EventNotification(mediaPlayer: self,
                                                          event: .changeRepeatMode))
    }
    
    func toggleShuffleMode() {
        let command = MPRemoteCommandCenter.shared().changeShuffleModeCommand
        switch shuffleState {
        case .none:
            command.currentShuffleType = .items
        case .items:
            command.currentShuffleType = .collections
        case .collection:
            command.currentShuffleType = .off
        }
        
        changeShuffleModeSubscriber.send(EventNotification(mediaPlayer: self,
                                                           event: .changeShuffleMode))
    }
    
    func toggleGravity() {
        switch playerLayer.videoGravity {
        case .resize:
            playerLayer.videoGravity = .resizeAspect
        case .resizeAspect:
            playerLayer.videoGravity = .resizeAspectFill
        case .resizeAspectFill:
            playerLayer.videoGravity = .resizeAspect
        default:
            assertionFailure("Invalid VideoPlayer Gravity")
        }
        
        playerGravitySubscriber.send(EventNotification(mediaPlayer: self,
                                                       event: .playerGravityChanged))
    }
    
    func setPlaybackRate(rate: Float) {
        previousRate = player.rate == 0.0 ? -1.0 : player.rate
        player.rate = rate
        changePlaybackRateSubscriber.send(EventNotification(mediaPlayer: self,
                                                            event: .changePlaybackRate))
    }
    
    func attachLayer() -> CALayer {
        playerLayer.player = player
        return playerLayer
    }
    
    func detachLayer() {
        playerLayer.player = nil
    }
    
    // MARK: - Private Variables
    
    private let player = AVPlayer().then {
        $0.seek(to: .zero)
        $0.actionAtItemEnd = .none
    }
    
    private let playerLayer = AVPlayerLayer().then {
        $0.videoGravity = .resizeAspect
        $0.needsDisplayOnBoundsChange = true
    }
    
    private var periodicTimeObserver: Any?
    private var notificationObservers = Set<AnyCancellable>()
    private let pauseSubscriber = PassthroughSubject<EventNotification, Never>()
    private let playSubscriber = PassthroughSubject<EventNotification, Never>()
    private let stopSubscriber = PassthroughSubject<EventNotification, Never>()
    private let changePlaybackRateSubscriber = PassthroughSubject<EventNotification, Never>()
    private let changeRepeatModeSubscriber = PassthroughSubject<EventNotification, Never>()
    private let changeShuffleModeSubscriber = PassthroughSubject<EventNotification, Never>()
    private let nextTrackSubscriber = PassthroughSubject<EventNotification, Never>()
    private let previousTrackSubscriber = PassthroughSubject<EventNotification, Never>()
    private let skipForwardSubscriber = PassthroughSubject<EventNotification, Never>()
    private let skipBackwardSubscriber = PassthroughSubject<EventNotification, Never>()
    private let seekForwardSubscriber = PassthroughSubject<EventNotification, Never>()
    private let seekBackwardSubscriber = PassthroughSubject<EventNotification, Never>()
    private let changePlaybackPositionSubscriber = PassthroughSubject<EventNotification, Never>()
    private let finishedPlayingSubscriber = PassthroughSubject<EventNotification, Never>()
    private let periodicTimeSubscriber = PassthroughSubject<EventNotification, Never>()
    private let pictureInPictureStatusSubscriber = PassthroughSubject<EventNotification, Never>()
    private let playerGravitySubscriber = PassthroughSubject<EventNotification, Never>()
}

extension MediaPlayer {
    enum Event {
        case pause
        case play
        case stop
        case changePlaybackRate
        case changeRepeatMode
        case changeShuffleMode
        case nextTrack
        case previousTrack
        case skipForward
        case skipBackward
        case seekForward
        case seekBackward
        case changePlaybackPosition
        case finishedPlaying
        case periodicPlayTimeChanged
        case pictureInPictureStatusChanged
        case playerGravityChanged
    }
    
    struct EventNotification {
        let mediaPlayer: MediaPlayer
        let event: Event
    }
    
    public func publisher(for event: Event) -> AnyPublisher<EventNotification, Never> {
        switch event {
        case .pause:
            return pauseSubscriber.eraseToAnyPublisher()
        case .play:
            return playSubscriber.eraseToAnyPublisher()
        case .stop:
            return stopSubscriber.eraseToAnyPublisher()
        case .changePlaybackRate:
            return changePlaybackRateSubscriber.eraseToAnyPublisher()
        case .changeRepeatMode:
            return changeRepeatModeSubscriber.eraseToAnyPublisher()
        case .changeShuffleMode:
            return changeShuffleModeSubscriber.eraseToAnyPublisher()
        case .nextTrack:
            return nextTrackSubscriber.eraseToAnyPublisher()
        case .previousTrack:
            return previousTrackSubscriber.eraseToAnyPublisher()
        case .skipForward:
            return skipForwardSubscriber.eraseToAnyPublisher()
        case .skipBackward:
            return skipBackwardSubscriber.eraseToAnyPublisher()
        case .seekForward:
            return seekForwardSubscriber.eraseToAnyPublisher()
        case .seekBackward:
            return seekBackwardSubscriber.eraseToAnyPublisher()
        case .changePlaybackPosition:
            return changePlaybackPositionSubscriber.eraseToAnyPublisher()
        case .finishedPlaying:
            return finishedPlayingSubscriber.eraseToAnyPublisher()
        case .periodicPlayTimeChanged:
            return periodicTimeSubscriber.eraseToAnyPublisher()
        case .pictureInPictureStatusChanged:
            return pictureInPictureStatusSubscriber.eraseToAnyPublisher()
        case .playerGravityChanged:
            return playerGravitySubscriber.eraseToAnyPublisher()
        }
    }
}

extension MediaPlayer {
    // Registers basic notifications
    private func registerNotifications() {
        NotificationCenter.default.publisher(for: UIApplication.didEnterBackgroundNotification)
        .receive(on: RunLoop.main)
        .sink { [weak self] _ in
            guard let self = self else { return }
            
            if let pictureInPictureController = self.pictureInPictureController,
               pictureInPictureController.isPictureInPictureActive {
                return
            }
            
            self.playerLayer.player = nil
        }.store(in: &notificationObservers)
        
        NotificationCenter.default.publisher(for: UIApplication.didBecomeActiveNotification)
        .receive(on: RunLoop.main)
        .sink { [weak self] _ in
            guard let self = self else { return }
            
            if let pictureInPictureController = self.pictureInPictureController,
               pictureInPictureController.isPictureInPictureActive {
                return
            }
            
            self.playerLayer.player = self.player
        }.store(in: &notificationObservers)
        
        NotificationCenter.default.publisher(for: .AVPlayerItemDidPlayToEndTime)
        .receive(on: RunLoop.main)
        .sink { [weak self] _ in
            guard let self = self else { return }
            
            self.finishedPlayingSubscriber.send(EventNotification(mediaPlayer: self,
                                                                  event: .finishedPlaying))
        }.store(in: &notificationObservers)
        
        let interval = CMTimeMake(value: 25, timescale: 1000)
        periodicTimeObserver = player.addPeriodicTimeObserver(forInterval: interval, queue: .main, using: { [weak self] time in
            guard let self = self else { return }
            
            self.periodicTimeSubscriber.send(EventNotification(mediaPlayer: self,
                                                               event: .periodicPlayTimeChanged))
        })
    }
    
    // Registers playback controls notifications
    private func registerControlCenterNotifications() {
        let center = MPRemoteCommandCenter.shared()
        center.publisher(for: .pauseCommand).sink { [weak self] _ in
            self?.pause()
        }.store(in: &notificationObservers)
        
        center.publisher(for: .playCommand).sink { [weak self] _ in
            self?.play()
        }.store(in: &notificationObservers)
        
        center.publisher(for: .stopCommand).sink { [weak self] _ in
            self?.stop()
        }.store(in: &notificationObservers)
        
        center.changePlaybackRateCommand.supportedPlaybackRates =
            supportedPlaybackRates.map { NSNumber(value: $0) }
        center.publisher(for: .changePlaybackRateCommand).sink { [weak self] event in
            guard let self = self, let event = event as? MPChangePlaybackRateCommandEvent else { return }
            self.setPlaybackRate(rate: event.playbackRate)
        }.store(in: &notificationObservers)
        
        center.publisher(for: .changeRepeatModeCommand).sink { [weak self] _ in
            self?.toggleRepeatMode()
        }.store(in: &notificationObservers)
        
        center.publisher(for: .changeShuffleModeCommand).sink { [weak self] _ in
            self?.toggleShuffleMode()
        }.store(in: &notificationObservers)
        
        center.publisher(for: .previousTrackCommand).sink { [weak self] _ in
            self?.seekPreviousTrack()
        }.store(in: &notificationObservers)
        
        center.publisher(for: .nextTrackCommand).sink { [weak self] _ in
            self?.seekNextTrack()
        }.store(in: &notificationObservers)
        
        center.skipBackwardCommand.preferredIntervals = [NSNumber(value: seekInterval)]
        center.publisher(for: .skipBackwardCommand).sink { [weak self] event in
            guard let self = self,
                  let event = event as? MPSkipIntervalCommandEvent else { return }
            
            let currentTime = self.player.currentTime()
            self.seekBackwards()
            MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyElapsedPlaybackTime] = Double(currentTime.seconds - event.interval)
        }.store(in: &notificationObservers)
        
        center.skipForwardCommand.preferredIntervals = [NSNumber(value: seekInterval)]
        center.publisher(for: .skipForwardCommand).sink { [weak self] event in
            guard let self = self,
                  let event = event as? MPSkipIntervalCommandEvent else { return }
            
            let currentTime = self.player.currentTime()
            self.seekForwards()
            MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyElapsedPlaybackTime] = Double(currentTime.seconds + event.interval)
        }.store(in: &notificationObservers)
        
        center.publisher(for: .changePlaybackPositionCommand).sink { [weak self] event in
            guard let self = self,
                  let event = event as? MPChangePlaybackPositionCommandEvent else { return }
            
            self.seek(to: event.positionTime)
        }.store(in: &notificationObservers)
    }
    
    // Registers picture in picture notifications
    private func registerPictureInPictureNotifications() {
        if AVPictureInPictureController.isPictureInPictureSupported() {
            pictureInPictureController = AVPictureInPictureController(playerLayer: self.playerLayer)
            guard let pictureInPictureController = pictureInPictureController else { return }
            
            pictureInPictureController.publisher(for: \AVPictureInPictureController.isPictureInPicturePossible).sink { [weak self] status in
                guard let self = self else { return }
                self.pictureInPictureStatusSubscriber.send(EventNotification(mediaPlayer: self,
                                                                             event: .pictureInPictureStatusChanged))
            }.store(in: &notificationObservers)
        } else {
            pictureInPictureStatusSubscriber.send(EventNotification(mediaPlayer: self,
                                                                    event: .pictureInPictureStatusChanged))
        }
    }
}

extension AVAsset {
    func isAudioTracksAvailable() -> Bool {
        tracks.filter({ $0.mediaType == .audio }).isEmpty == false
    }

    // If called on optional, assume true
    // We do this because for m3u8 HLS streams,
    // tracks may not always be available and the particle effect will show even on videos..
    // It's best to assume this type of media is a video stream.
    func isVideoTracksAvailable() -> Bool {
        tracks.isEmpty || tracks.filter({ $0.mediaType == .video }).isEmpty == false
    }
}
