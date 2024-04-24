// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AVKit
import Combine
import Foundation
import MediaPlayer
import SwiftUI
import os

@available(iOS 16.0, *)
final class PlayerModel: ObservableObject {

  init() {
    // FIXME: Consider moving this to a setUp method and call explicitly from UI
    player.seek(to: .zero)
    player.actionAtItemEnd = .none

    playerLayer.videoGravity = .resizeAspect
    playerLayer.needsDisplayOnBoundsChange = true
    playerLayer.player = player

    setupPlayerKeyPathObservation()
    setupRemoteCommandCenterHandlers()
    setupPlayerNotifications()
    updateSystemPlayer()
  }

  deinit {
    // FIXME: Make sure we call stop in UI layer onDisppear to avoid 17.0-17.2 bug
    if isPlaying {
      log.warning("PlayerModel deallocated without first stopping the underlying media.")
      stop()
    }
  }

  private let log = Logger(subsystem: Bundle.main.bundleIdentifier!, category: "PlayerModel")

  // MARK: - Player Controls

  var isPlaying: Bool {
    get {
      player.timeControlStatus == .playing
    }
    set {
      if newValue {
        play()
      } else {
        pause()
      }
    }
  }

  var currentTime: TimeInterval {
    guard let item = player.currentItem else { return 0 }
    return max(0, min(duration, item.currentTime().seconds))
  }

  var duration: TimeInterval {
    guard let item = player.currentItem else { return 0 }
    let seconds = CMTimeConvertScale(
      item.asset.duration,
      timescale: item.currentTime().timescale,
      method: .roundHalfAwayFromZero
    ).seconds
    return seconds.isNaN ? 0.0 : seconds
  }

  func play() {
    if isPlaying {
      return
    }
    if currentTime == duration {
      player.seek(to: .zero)
    }
    player.play()
  }

  var currentTimeStream: AsyncStream<TimeInterval> {
    return .init { [weak self] continuation in
      guard let self else { return }
      let observer = player.addCancellablePeriodicTimeObserver(forInterval: 500) { [weak self] _ in
        guard let self else { return }
        continuation.yield(currentTime)
      }
      // Should be tied to the View, but adding ane extra killswitch
      self.cancellables.insert(observer)
      continuation.onTermination = { _ in
        observer.cancel()
      }
    }
  }

  var didPlayToEndStream: AsyncStream<Void> {
    return .init { [weak self] continuation in
      guard let self else { return }
      let observer = NotificationCenter.default.addObserver(
        forName: AVPlayerItem.didPlayToEndTimeNotification,
        object: nil,
        queue: .main
      ) { [weak self] _ in
        guard let self else { return }
        continuation.yield()
      }
      let cancellable = AnyCancellable {
        _ = observer
      }
      // Should be tied to the View, but adding ane extra killswitch
      self.cancellables.insert(cancellable)
      continuation.onTermination = { _ in
        cancellable.cancel()
      }
    }
  }

  func pause() {
    if !isPlaying {
      return
    }
    player.pause()
  }

  func stop() {
    if !isPlaying {
      return
    }
    player.pause()
    player.replaceCurrentItem(with: nil)
  }

  func playPreviousTrack() {
    // Implement
  }

  func playNextTrack() {
    // Implement
  }

  private let seekInterval: TimeInterval = 15.0

  func seekBackwards() async {
    guard let currentItem = player.currentItem else {
      return
    }
    await seek(to: currentItem.currentTime().seconds - seekInterval)
  }

  func seekForwards() async {
    guard let currentItem = player.currentItem else {
      return
    }
    await seek(to: currentItem.currentTime().seconds + seekInterval)
  }

  func seek(to time: TimeInterval) async {
    guard let currentItem = player.currentItem else {
      return
    }
    let seekTime = CMTimeMakeWithSeconds(
      max(0.0, min(currentItem.duration.seconds, time)),
      preferredTimescale: currentItem.currentTime().timescale
    )
    await player.seek(to: seekTime)
  }

  // MARK: - Playback Extras

  enum RepeatMode {
    case none
    case one
    case all

    mutating func cycle() {
      switch self {
      case .none: self = .one
      case .one: self = .all
      case .all: self = .none
      }
    }
  }

  @Published var repeatMode: RepeatMode = .none

  @Published var isShuffleEnabled: Bool = false

  struct PlaybackSpeed: Equatable {
    var rate: Float
    var braveSystemName: String

    static let normal = Self(rate: 1.0, braveSystemName: "leo.1x")
    static let fast = Self(rate: 1.5, braveSystemName: "leo.1.5x")
    static let faster = Self(rate: 2.0, braveSystemName: "leo.2x")

    mutating func cycle() {
      switch self {
      case .normal: self = .fast
      case .fast: self = .faster
      case .faster: self = .normal
      default: self = .fast
      }
    }
  }

  @Published var playbackSpeed: PlaybackSpeed = .normal {
    didSet {
      player.rate = playbackSpeed.rate
      player.defaultRate = playbackSpeed.rate
    }
  }

  // MARK: - UI

  /// Whether or not the video that is currently loaded into the `AVPlayer` is a potrait video
  // FIXME: Have to hook this up somehow to loading video tracks to get naturalSize
  private(set) var isPortraitVideo: Bool = false

  // MARK: - Picture in Picture

  private var pipController: AVPictureInPictureController?

  // FIXME: Maybe update this based on AVPictureInPictureController.isPictureInPicturePossible KVO
  @Published private(set) var isPictureInPictureSupported: Bool =
    AVPictureInPictureController.isPictureInPictureSupported()

  func togglePictureInPicture() {
    pipController = AVPictureInPictureController(playerLayer: playerLayer)
    pipController?.startPictureInPicture()
  }

  // MARK: - AirPlay

  // MARK: -

  let player: AVQueuePlayer = .init()
  private let playerLayer: AVPlayerLayer = .init()

  private var cancellables: Set<AnyCancellable> = []

  /// Sets up KVO observations for AVPlayer properties which trigger the `objectWillChange` publisher
  private func setupPlayerKeyPathObservation() {
    func subscriber<Value>(for keyPath: KeyPath<AVQueuePlayer, Value>) -> AnyCancellable {
      let observation = player.observe(keyPath, options: [.prior]) { [weak self] _, change in
        if change.isPrior {
          self?.objectWillChange.send()
        }
      }
      return .init {
        observation.invalidate()
      }
    }
    cancellables.formUnion([
      subscriber(for: \.timeControlStatus)
      // FIXME: Add the rest
    ])
  }

  private func setupPlayerNotifications() {
    let nc = NotificationCenter.default
    let didPlayToEndTimeNotification = nc.addObserver(
      forName: AVPlayerItem.didPlayToEndTimeNotification,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self else { return }
      player.pause()
    }
    cancellables.insert(
      .init {
        _ = didPlayToEndTimeNotification
      }
    )
  }
}

extension AVPlayer {
  fileprivate func addCancellablePeriodicTimeObserver(
    forInterval interval: TimeInterval,
    using block: @escaping @Sendable (CMTime) -> Void
  ) -> AnyCancellable {
    let observer = addPeriodicTimeObserver(
      forInterval: CMTime(value: Int64(interval), timescale: 1000),
      queue: .main,
      using: block
    )
    return .init {
      self.removeTimeObserver(observer)
    }
  }
}

// MARK: - System Media Player

@available(iOS 16.0, *)
extension PlayerModel {
  private func updateSystemPlayer() {
    let remoteCommandsCenter: MPRemoteCommandCenter = .shared()
    remoteCommandsCenter.skipBackwardCommand.preferredIntervals = [.init(value: seekInterval)]
    remoteCommandsCenter.skipForwardCommand.preferredIntervals = [.init(value: seekInterval)]

    let nowPlayingCenter: MPNowPlayingInfoCenter = .default()
    // FIXME: Check if this is enough
    nowPlayingCenter.nowPlayingInfo = player.currentItem?.nowPlayingInfo
  }

  private func setupRemoteCommandCenterHandlers() {
    let center: MPRemoteCommandCenter = .shared()
    let commands: [MPRemoteCommand] = [
      center.pauseCommand,
      center.playCommand,
      center.stopCommand,
      // FIXME: Add the rest
    ]
    cancellables.formUnion(
      commands.map {
        $0.addCancellableTarget { [weak self] event in
          self?.handleControlCenterCommand(event) ?? .noSuchContent
        }
      }
    )
  }

  private func handleControlCenterCommand(
    _ event: MPRemoteCommandEvent
  ) -> MPRemoteCommandHandlerStatus {
    let center: MPRemoteCommandCenter = .shared()
    switch event.command {
    case center.pauseCommand:
      pause()
    case center.playCommand:
      play()
    case center.stopCommand:
      stop()
    default:
      break
    }
    return .success
  }
}

extension MPRemoteCommand {
  fileprivate func addCancellableTarget(
    handler: @escaping (MPRemoteCommandEvent) -> MPRemoteCommandHandlerStatus
  ) -> AnyCancellable {
    let handle = addTarget(handler: handler)
    return .init {
      self.removeTarget(handle)
    }
  }
}
