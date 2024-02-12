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
import os.log
import Then
import UserAgent

public enum MediaPlaybackError: Error {
  case cancelled
  case cannotLoadAsset(status: AVKeyValueStatus)
  case other(Error)
}

public class MediaPlayer: NSObject {
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
  private(set) public var repeatState: RepeatMode = .none
  private(set) public var shuffleState: ShuffleMode = .none
  private(set) var previousRate: Float = 0.0

  public var isPlaying: Bool {
    // It is better NOT to keep tracking of isPlaying OR rate > 0.0
    // Instead we should use the timeControlStatus because PIP and Background play
    // via control-center will modify the timeControlStatus property
    // This will keep our UI consistent with what is on the lock-screen.
    // This will also allow us to properly determine play state in
    // PlaylistMediaInfo -> init -> MPRemoteCommandCenter.shared().playCommand
    player.timeControlStatus == .playing
  }

  public var isWaitingToPlay: Bool {
    player.timeControlStatus == .waitingToPlayAtSpecifiedRate
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

  override public init() {
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
      Logger.module.error("\(error.localizedDescription)")
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
      Logger.module.error("\(error.localizedDescription)")
    }

    // Stop receiving remote commands
    UIApplication.shared.endReceivingRemoteControlEvents()
  }
  
  public func clear() {
    player.replaceCurrentItem(with: nil)
    pendingMediaItem = nil
  }

  public func load(url: URL) async throws -> Bool {
    try await load(asset: AVURLAsset(url: url, options: AVAsset.defaultOptions))
  }

  /// On success, returns a publisher with a Boolean.
  /// The boolean indicates if a NEW player item was loaded, OR if an existing item was loaded.
  /// If an existing item is loaded, you should seek to offset zero to restart playback.
  /// If a new item is loaded, you should call play to begin playback.
  /// Returns an error on failure.
  public func load(asset: AVURLAsset) async throws -> Bool {
    // If the same asset is being loaded again.
    // Just play it.
    if let currentItem = player.currentItem, currentItem.asset.isKind(of: AVURLAsset.self) && player.status == .readyToPlay {
      if let currentAsset = currentItem.asset as? AVURLAsset, currentAsset.url.absoluteString == asset.url.absoluteString {
        pendingMediaItem = nil
        return false
      }
    }

    let item = AVPlayerItem(asset: asset)
    pendingMediaItem = item
    
    _ = try await asset.load(.isPlayable, .tracks, .duration)

    player.replaceCurrentItem(with: item)
    pendingMediaItem = nil
    return true  // New Item loaded
  }

  public func play() {
    if !isPlaying {
      player.play()
      
      if #unavailable(iOS 16) {
        player.rate = previousRate > 0.0 ? previousRate : 1.0
      }
      playSubscriber.send(EventNotification(mediaPlayer: self, event: .play))
    }
  }

  public func pause() {
    if isPlaying {
      if #unavailable(iOS 16) {
        previousRate = player.rate
      }
      player.pause()
      pauseSubscriber.send(EventNotification(mediaPlayer: self, event: .pause))
    }
  }

  public func stop() {
    if isPlaying {
      if #unavailable(iOS 16) {
        previousRate = player.rate
      }
      player.pause()
      player.replaceCurrentItem(with: nil)
      stopSubscriber.send(EventNotification(mediaPlayer: self, event: .stop))
    }
  }

  public func seekPreviousTrack() {
    previousTrackSubscriber.send(EventNotification(mediaPlayer: self, event: .previousTrack))
  }

  public func seekNextTrack() {
    nextTrackSubscriber.send(EventNotification(mediaPlayer: self, event: .nextTrack))
  }

  public func seekBackwards() {
    if let currentItem = player.currentItem {
      let currentTime = currentItem.currentTime().seconds
      var seekTime = currentTime - seekInterval

      if seekTime < 0 {
        seekTime = 0
      }

      let absoluteTime = CMTimeMakeWithSeconds(seekTime, preferredTimescale: currentItem.currentTime().timescale)

      // Seeking to .zero, .zero can cause a performance hit
      // So give a nice tolerance to seeking
      player.seek(to: absoluteTime, toleranceBefore: .positiveInfinity, toleranceAfter: .positiveInfinity)

      seekBackwardSubscriber.send(EventNotification(mediaPlayer: self, event: .seekBackward))
    }
  }

  public func seekForwards() {
    if let currentItem = player.currentItem {
      let currentTime = currentItem.currentTime().seconds
      let seekTime = currentTime + seekInterval

      if seekTime < (currentItem.duration.seconds - seekInterval) {
        let absoluteTime = CMTimeMakeWithSeconds(seekTime, preferredTimescale: currentItem.currentTime().timescale)

        // Seeking to .zero, .zero can cause a performance hit
        // So give a nice tolerance to seeking
        player.seek(to: absoluteTime, toleranceBefore: .positiveInfinity, toleranceAfter: .positiveInfinity)

        seekForwardSubscriber.send(EventNotification(mediaPlayer: self, event: .seekForward))
      }
    }
  }

  public func seek(to time: TimeInterval) {
    if let currentItem = player.currentItem {
      var seekTime = time
      if seekTime < 0.0 {
        seekTime = 0.0
      }

      if seekTime >= currentItem.duration.seconds {
        seekTime = currentItem.duration.seconds
      }

      let absoluteTime = CMTimeMakeWithSeconds(seekTime, preferredTimescale: currentItem.currentTime().timescale)

      // Seeking to .zero, .zero can cause a performance hit
      // So give a nice tolerance to seeking
      player.seek(to: absoluteTime, toleranceBefore: .positiveInfinity, toleranceAfter: .positiveInfinity)

      self.changePlaybackPositionSubscriber.send(
        EventNotification(
          mediaPlayer: self,
          event: .changePlaybackPosition))
    }
  }

  public func toggleRepeatMode() {
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

    changeRepeatModeSubscriber.send(
      EventNotification(
        mediaPlayer: self,
        event: .changeRepeatMode))
  }

  public func toggleShuffleMode() {
    let command = MPRemoteCommandCenter.shared().changeShuffleModeCommand
    switch shuffleState {
    case .none:
      command.currentShuffleType = .items
    case .items:
      command.currentShuffleType = .collections
    case .collection:
      command.currentShuffleType = .off
    }

    changeShuffleModeSubscriber.send(
      EventNotification(
        mediaPlayer: self,
        event: .changeShuffleMode))
  }

  public func toggleGravity() {
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

    playerGravitySubscriber.send(
      EventNotification(
        mediaPlayer: self,
        event: .playerGravityChanged))
  }

  public func setPlaybackRate(rate: Float) {
    if #available(iOS 16, *) {
      player.defaultRate = rate
      player.rate = rate
    } else {
      previousRate = player.rate
      player.rate = rate
    }
    
    changePlaybackRateSubscriber.send(
      EventNotification(
        mediaPlayer: self,
        event: .changePlaybackRate))
  }

  @discardableResult
  public func attachLayer() -> CALayer {
    playerLayer.player = player
    return playerLayer
  }
  
  public func addTimeObserver(interval: Int, onTick: @escaping (CMTime) -> Void) -> Any {
    let interval = CMTimeMake(value: Int64(interval), timescale: 1000)
    return player.addPeriodicTimeObserver(
      forInterval: interval, queue: .main,
      using: { time in
        onTick(time)
      })
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
  private var rateObserver: AnyObject?
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
  public enum Event {
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

  public struct EventNotification {
    public let mediaPlayer: MediaPlayer
    public let event: Event
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
  /// Registers basic notifications
  private func registerNotifications() {
    NotificationCenter.default.publisher(for: UIApplication.didEnterBackgroundNotification)
      .sink { [weak self] _ in
      guard let self = self else { return }

      if let pictureInPictureController = self.pictureInPictureController,
        pictureInPictureController.isPictureInPictureActive {
        return
      }

      self.playerLayer.player = nil
    }.store(in: &notificationObservers)

    NotificationCenter.default.publisher(for: UIApplication.willEnterForegroundNotification)
      .sink { [weak self] _ in
      guard let self = self else { return }

      if let pictureInPictureController = self.pictureInPictureController,
        pictureInPictureController.isPictureInPictureActive {
        return
      }

      self.playerLayer.player = self.player
    }.store(in: &notificationObservers)
    
    NotificationCenter.default.publisher(for: AVAudioSession.interruptionNotification, object: AVAudioSession.sharedInstance())
      .sink { [weak self] notification in

        guard let self = self,
          let userInfo = notification.userInfo,
          let typeValue = userInfo[AVAudioSessionInterruptionTypeKey] as? UInt,
          let type = AVAudioSession.InterruptionType(rawValue: typeValue)
        else {

          return
        }

        switch type {
        case .began:
          // An interruption began. Update the UI as necessary.
          self.pause()

        case .ended:
          // An interruption ended. Resume playback, if appropriate.
          guard let optionsValue = userInfo[AVAudioSessionInterruptionOptionKey] as? UInt else {
            return
          }

          let options = AVAudioSession.InterruptionOptions(rawValue: optionsValue)
          if options.contains(.shouldResume) {
            // An interruption ended. Resume playback.
            self.play()
          } else {
            // An interruption ended. Don't resume playback.
            Logger.module.debug("Interuption ended, but suggests not to resume playback.")
          }

        default:
          break
        }
      }.store(in: &notificationObservers)

    NotificationCenter.default.publisher(for: .AVPlayerItemDidPlayToEndTime)
      .sink { [weak self] _ in
        guard let self = self else { return }

        self.finishedPlayingSubscriber.send(
          EventNotification(
            mediaPlayer: self,
            event: .finishedPlaying))
      }.store(in: &notificationObservers)

    periodicTimeObserver = addTimeObserver(interval: 25, onTick: { [weak self] _ in
      guard let self = self else { return }

      self.periodicTimeSubscriber.send(
        EventNotification(
          mediaPlayer: self,
          event: .periodicPlayTimeChanged))
    })
  }

  /// Registers playback controls notifications
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
        let event = event as? MPSkipIntervalCommandEvent
      else { return }

      let currentTime = self.player.currentTime()
      self.seekBackwards()
      MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyElapsedPlaybackTime] = Double(currentTime.seconds - event.interval)
    }.store(in: &notificationObservers)

    center.skipForwardCommand.preferredIntervals = [NSNumber(value: seekInterval)]
    center.publisher(for: .skipForwardCommand).sink { [weak self] event in
      guard let self = self,
        let event = event as? MPSkipIntervalCommandEvent
      else { return }

      let currentTime = self.player.currentTime()
      self.seekForwards()
      MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyElapsedPlaybackTime] = Double(currentTime.seconds + event.interval)
    }.store(in: &notificationObservers)
    
    center.publisher(for: .seekBackwardCommand).sink { [weak self] event in
      guard let self = self,
        let event = event as? MPSkipIntervalCommandEvent
      else { return }

      let currentTime = self.player.currentTime()
      self.seekBackwards()
      MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyElapsedPlaybackTime] = Double(currentTime.seconds - event.interval)
    }.store(in: &notificationObservers)

    center.publisher(for: .seekForwardCommand).sink { [weak self] event in
      guard let self = self,
        let event = event as? MPSkipIntervalCommandEvent
      else { return }

      let currentTime = self.player.currentTime()
      self.seekForwards()
      MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPNowPlayingInfoPropertyElapsedPlaybackTime] = Double(currentTime.seconds + event.interval)
    }.store(in: &notificationObservers)

    center.publisher(for: .changePlaybackPositionCommand).sink { [weak self] event in
      guard let self = self,
        let event = event as? MPChangePlaybackPositionCommandEvent
      else { return }

      self.seek(to: event.positionTime)
    }.store(in: &notificationObservers)
    
    // The following code is simulating on iOS <= 15: https://developer.apple.com/documentation/avfoundation/avplayer/3929373-defaultrate
    // When entering `PictureInPicture`, we have no way of knowing if the user has PAUSED or PLAYED the video/audio while in PIP
    // The only way to know, is to observe the `rate`.
    // However, setting the rate back to the default rate will recursively call the observer that's observing PIP.
    // So we need to do some weird hacks below.
    // On iOS 16+, we can use `defaultRate` variable instead of storing `previousRate`
    if #unavailable(iOS 16) {
      var isRecursivelySettingRate = false
      rateObserver = player.observe(\.rate, options: [.new, .prior]) { [weak self] player, rate in
        guard let self = self else { return }
        
        if !isRecursivelySettingRate {
          if rate.isPrior {
            if player.rate != 0 {
              previousRate = player.rate
            }
            return
          }
          
          if self.pictureInPictureController?.isPictureInPictureActive == true {
            if rate.newValue == 1 && self.previousRate != rate.newValue {
              DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                isRecursivelySettingRate = true
                player.rate = self.previousRate
                isRecursivelySettingRate = false
              }
            }
          }
        }
        
        changePlaybackRateSubscriber.send(
          EventNotification(
            mediaPlayer: self,
            event: .changePlaybackRate))
      }
    }
  }

  /// Registers picture in picture notifications
  private func registerPictureInPictureNotifications() {
    if AVPictureInPictureController.isPictureInPictureSupported() {
      pictureInPictureController = AVPictureInPictureController(playerLayer: self.playerLayer)
      guard let pictureInPictureController = pictureInPictureController else { return }

      pictureInPictureController.publisher(for: \AVPictureInPictureController.isPictureInPicturePossible).sink { [weak self] status in
        guard let self = self else { return }
        self.pictureInPictureStatusSubscriber.send(
          EventNotification(
            mediaPlayer: self,
            event: .pictureInPictureStatusChanged))
      }.store(in: &notificationObservers)
    } else {
      pictureInPictureStatusSubscriber.send(
        EventNotification(
          mediaPlayer: self,
          event: .pictureInPictureStatusChanged))
    }
  }
}

extension AVPlayerItem {
  private var isReadyToPlay: Bool {
    var error: NSError?
    if case .loaded = self.asset.statusOfValue(forKey: "tracks", error: &error) {
      return true
    }
    return false
  }
  
  /// Returns whether or not the assetTrack has audio tracks OR the asset has audio tracks
  public func isAudioTracksAvailable() -> Bool {
    tracks.filter({ $0.assetTrack?.mediaType == .audio }).isEmpty == false
  }

  /// Returns whether or not the assetTrack has video tracks OR the asset has video tracks
  /// If called on optional, assume true
  /// We do this because for m3u8 HLS streams,
  /// tracks may not always be available and the particle effect will show even on videos..
  /// It's best to assume this type of media is a video stream.
  public func isVideoTracksAvailable() -> Bool {
    if !isReadyToPlay {
      return true
    }
    
    if tracks.isEmpty && asset.tracks.isEmpty {
      return true  // Assume video
    }
    
    // All tracks are null (not loaded yet)
    if tracks.allSatisfy({ $0.assetTrack == nil }) {
      return true  // Assume video
    }
    
    // If the only current track types are audio
    if !tracks.allSatisfy({ $0.assetTrack?.mediaType == .audio }) {
      return true  // Assume video
    }
    
    let hasVideoTracks = !tracks.filter({ $0.assetTrack?.mediaType == .video }).isEmpty || asset.isVideoTracksAvailable()
    
    // Ultra hack
    // Some items `fade` in/out or have an audio track that fades out but no video track
    // In this case, assume video as it is potentially still a video, just with blank frames
    if !hasVideoTracks &&
        currentTime().seconds <= 1.0 ||
        fabs(duration.seconds - currentTime().seconds) <= 3.0 {
      return true
    }
    
    return hasVideoTracks
  }
}

extension AVAsset {
  /// Returns whether or not the asset has audio tracks
  public func isAudioTracksAvailable() -> Bool {
    !tracks.filter({ $0.mediaType == .audio }).isEmpty
  }

  /// Returns whether or not the  asset has video tracks
  /// If called on optional, assume true
  /// We do this because for m3u8 HLS streams,
  /// tracks may not always be available and the particle effect will show even on videos..
  /// It's best to assume this type of media is a video stream.
  public func isVideoTracksAvailable() -> Bool {
    !tracks.filter({ $0.mediaType == .video }).isEmpty
  }
  
  public static var defaultOptions: [String: Any] {
    let userAgent = UserAgent.shouldUseDesktopMode ? UserAgent.desktop : UserAgent.mobile
    var options: [String: Any] = [:]
    if #available(iOS 16, *) {
        options[AVURLAssetHTTPUserAgentKey] = userAgent
    } else {
        options["AVURLAssetHTTPHeaderFieldsKey"] = ["User-Agent": userAgent]
    }
    return options
  }
}
