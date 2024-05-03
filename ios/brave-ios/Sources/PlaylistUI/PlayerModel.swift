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
    updateSystemPlayer()

    DispatchQueue.global().async {
      // should mode be `.moviePlayback`?
      try? AVAudioSession.sharedInstance().setCategory(.playback, mode: .default)
    }
  }

  deinit {
    if isPlaying {
      log.warning("PlayerModel deallocated without first stopping the underlying media.")
      stop()
    }
  }

  private let log = Logger(subsystem: Bundle.main.bundleIdentifier!, category: "PlayerModel")

  // MARK: - Player Status

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
      ) { _ in
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

  // MARK: - Player Controls

  func play() {
    if isPlaying {
      return
    }
    if currentTime == duration {
      player.seek(to: .zero)
    }
    player.play()
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
    item = nil
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

  func seek(to time: TimeInterval, accurately: Bool = false) async {
    guard let currentItem = player.currentItem else {
      return
    }
    let seekTime = CMTimeMakeWithSeconds(
      max(0.0, min(currentItem.duration.seconds, time)),
      preferredTimescale: currentItem.currentTime().timescale
    )
    await player.seek(
      to: seekTime,
      toleranceBefore: accurately ? .zero : .positiveInfinity,
      toleranceAfter: accurately ? .zero : .positiveInfinity
    )
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

  private var sleepTimer: Timer?
  @Published var sleepTimerFireDate: Date? {
    didSet {
      sleepTimer?.invalidate()
      if let sleepTimerFireDate {
        let timer = Timer(
          fire: sleepTimerFireDate,
          interval: 0,
          repeats: false,
          block: { [weak self] _ in
            self?.pause()
            self?.sleepTimerFireDate = nil
          }
        )
        RunLoop.main.add(timer, forMode: .default)
        self.sleepTimer = timer
      }
    }
  }

  // MARK: - UI

  /// Whether or not the video that is currently loaded into the `AVPlayer` is a potrait video
  var isPortraitVideo: Bool {
    guard let item = player.currentItem else { return false }
    return item.presentationSize.height > item.presentationSize.width
  }

  /// The aspect ratio of the current video
  var aspectRatio: Double {
    guard let item = player.currentItem else { return 16 / 9 }
    return item.presentationSize.width / item.presentationSize.height
  }

  /// A stream that yields downsampled thumbnails of the item currently playing.
  var videoAmbianceImageStream: AsyncStream<UIImage> {
    return .init { [weak self] continuation in
      guard let self else { return }
      let timeObserver = player.addCancellablePeriodicTimeObserver(
        forInterval: 100,
        queue: .global()
      ) { [weak self] time in
        guard let self,
          self.isPlaying,
          let buffer = self.videoDecorationOutput.copyPixelBuffer(
            forItemTime: time,
            itemTimeForDisplay: nil
          )
        else {
          return
        }
        let ciImage = CIImage(cvPixelBuffer: buffer)
          .transformed(by: .init(scaleX: 0.1, y: 0.1), highQualityDownsample: false)
        if let cgImage = CIContext().createCGImage(ciImage, from: ciImage.extent) {
          let uiImage = UIImage(cgImage: cgImage)
          DispatchQueue.main.async {
            continuation.yield(uiImage)
          }
        }
      }
      // Should be tied to the View, but adding ane extra killswitch
      self.cancellables.insert(timeObserver)
      continuation.onTermination = { _ in
        timeObserver.cancel()
      }
    }
  }

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

  var allowsExternalPlayback: Bool {
    player.allowsExternalPlayback
  }

  // MARK: -

  let player: AVPlayer = .init()
  private let videoDecorationOutput = AVPlayerItemVideoOutput(pixelBufferAttributes: [
    kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA
  ])
  var item: AVPlayerItem? {
    get {
      player.currentItem
    }
    set {
      player.replaceCurrentItem(with: newValue)
      setupPlayerItemKeyPathObservation()
      newValue?.add(videoDecorationOutput)
    }
  }
  private let playerLayer: AVPlayerLayer = .init()

  private var cancellables: Set<AnyCancellable> = []
  private var itemCancellables: Set<AnyCancellable> = []

  /// Sets up KVO observations for AVPlayer properties which trigger the `objectWillChange` publisher
  private func setupPlayerKeyPathObservation() {
    func subscriber<Value>(for keyPath: KeyPath<AVPlayer, Value>) -> AnyCancellable {
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
      subscriber(for: \.timeControlStatus),
      subscriber(for: \.allowsExternalPlayback),
      // FIXME: Add the rest
    ])
  }

  private func setupPlayerItemKeyPathObservation() {
    itemCancellables.removeAll()
    guard let item = item else { return }
    func subscriber<Value>(for keyPath: KeyPath<AVPlayerItem, Value>) -> AnyCancellable {
      let observation = item.observe(keyPath, options: [.prior]) { [weak self] _, change in
        if change.isPrior {
          self?.objectWillChange.send()
        }
      }
      return .init {
        observation.invalidate()
      }
    }
    itemCancellables.formUnion([
      subscriber(for: \.presentationSize)
    ])
  }
}

extension AVPlayer {
  fileprivate func addCancellablePeriodicTimeObserver(
    forInterval interval: TimeInterval,
    queue: DispatchQueue = .main,
    using block: @escaping @Sendable (CMTime) -> Void
  ) -> AnyCancellable {
    let observer = addPeriodicTimeObserver(
      forInterval: CMTime(value: Int64(interval), timescale: 1000),
      queue: queue,
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
