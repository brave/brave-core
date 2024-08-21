// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import AVKit
import BraveCore
import BraveUI
import Preferences
import Shared
import SnapKit
import UIKit

// The NewTabPageVideoAdPlayer class is responsible for handling video ads on a
// new tab page. It sets up the video player, controls playback (play, pause,
// stop), and manages user interactions. The class also listens for events like
// video start, pause, and completion, and tracks ad performance metrics.
class NewTabPageVideoAdPlayer {
  var didStartAutoplayEvent: (() -> Void)?
  var didFinishAutoplayEvent: (() -> Void)?
  var didStartPlaybackEvent: (() -> Void)?
  var didFinishPlaybackEvent: (() -> Void)?
  var didPlay25PercentEvent: (() -> Void)?
  var didCancelPlaybackEvent: (() -> Void)?

  var player: AVPlayer?

  private let backgroundVideoPath: URL
  private var didLoadVideoAsset = false
  private var didFinishAutoplay = false

  private var media25TimeObserver: Any?
  private var media100TimeObserver: Any?
  private var autoplayPausedObserver: NSKeyValueObservation?

  private let kMaxAutoplayDurationInSeconds = 6.0
  private let kStopFramePositionAdjustment = 0.5

  private var duration: CMTime?
  private var frameRate: Double?

  private var isPlaying: Bool {
    player?.timeControlStatus != .paused
  }

  init(_ backgroundVideoPath: URL) {
    self.backgroundVideoPath = backgroundVideoPath
  }

  deinit {
    removeObservers()
  }

  func resetPlayer() {
    player = nil
  }

  func createPlayer() {
    let item = AVPlayerItem(url: backgroundVideoPath)
    player = AVPlayer(playerItem: item)
    player?.actionAtItemEnd = .pause
  }

  func startPlayback() {
    finishAutoplayIfNeeded(shouldSeekToStopFrame: false)

    seekToBeginning { [weak self] in
      guard let self else { return }

      didStartPlaybackEvent?()

      addMediaObservers()

      if isPlaylistActive() {
        PlaylistCoordinator.shared.mediaPlayer?.pause()
      }

      player?.isMuted = false
      player?.play()
    }
  }

  func seekToStopFrame() {
    player?.seek(to: stopFrameTime(), toleranceBefore: CMTime.zero, toleranceAfter: CMTime.zero)
  }

  func cancelPlayIfNeeded() {
    finishAutoplayIfNeeded()

    removeObservers()
    player?.pause()

    didCancelPlaybackEvent?()
  }

  func togglePlay() -> Bool {
    let wasPlaying = isPlaying

    if wasPlaying {
      player?.pause()
    } else {
      player?.play()
    }

    return !wasPlaying
  }

  func loadAndAutoplayVideoAssetIfNeeded(shouldAutoplay: Bool) {
    if !didLoadVideoAsset {
      didLoadVideoAsset = true
      loadTrackParams(backgroundVideoPath, shouldAutoplay: shouldAutoplay)
    }
  }

  private func seekToBeginning(completion: @escaping () -> Void) {
    player?.seek(to: CMTime.zero) { _ in
      completion()
    }
  }

  private func stopFrameTime() -> CMTime {
    var stopFrameTime = calculateStopFrameTime()

    if let duration, stopFrameTime > duration {
      stopFrameTime = duration
    }

    return stopFrameTime
  }

  private func calculateStopFrameTime() -> CMTime {
    var stopFrameTime = CMTime(
      seconds: kMaxAutoplayDurationInSeconds,
      preferredTimescale: CMTimeScale(NSEC_PER_SEC)
    )

    let stopFrame = parseStopFrameFromFilename(
      filename: backgroundVideoPath.lastPathComponent
    )
    if let stopFrame, let frameRate {
      // Adjust the stop frame time to the middle of the stop frame.
      let stopFrameTimeInSeconds = (stopFrame + kStopFramePositionAdjustment) / frameRate
      if stopFrameTimeInSeconds < kMaxAutoplayDurationInSeconds {
        stopFrameTime = CMTime(
          seconds: stopFrameTimeInSeconds,
          preferredTimescale: CMTimeScale(NSEC_PER_SEC)
        )
      }
    }

    return stopFrameTime
  }

  private func loadTrackParams(_ backgroundVideoPath: URL, shouldAutoplay: Bool) {
    Task {
      do {
        var frameRate: Float?
        let asset: AVURLAsset = AVURLAsset(url: backgroundVideoPath)
        let (isPlayable, duration) = try await asset.load(.isPlayable, .duration)
        if isPlayable {
          if let track = try? await asset.loadTracks(withMediaType: .video).first {
            frameRate = try? await track.load(.nominalFrameRate)
          }
          await loadedTrackParams(
            duration: duration,
            frameRate: frameRate,
            shouldAutoplay: shouldAutoplay
          )
        }
      } catch {}
    }
  }

  @MainActor
  private func loadedTrackParams(
    duration: CMTime?,
    frameRate: Float?,
    shouldAutoplay: Bool
  ) async {
    self.duration = duration
    if let frameRate {
      self.frameRate = Double(frameRate)
    }

    if !shouldAutoplay || isPlaylistActive() {
      finishAutoplayIfNeeded(didStartAutoplay: false)
      return
    }

    startAutoplay()
  }

  private func startAutoplay() {
    let forwardPlaybackEndTime = stopFrameTime()
    // If autoplay length is less then video duration then set forward playback
    // end time to stop the video at the specified time.
    if let duration, forwardPlaybackEndTime < duration {
      player?.currentItem?.forwardPlaybackEndTime = forwardPlaybackEndTime
    }

    didStartAutoplayEvent?()

    addAutoplayPausedObserver()

    allowBackgroundAudioDuringAutoplay()

    player?.isMuted = true
    player?.play()
  }

  private func finishAutoplayIfNeeded(shouldSeekToStopFrame: Bool = true, didStartAutoplay: Bool = true) {
    if didFinishAutoplay {
      return
    }
    didFinishAutoplay = true

    removeAutoplayPausedObserver()
    player?.currentItem?.forwardPlaybackEndTime = CMTime()
    player?.pause()

    if shouldSeekToStopFrame {
      seekToStopFrame()
    }

    if didStartAutoplay {
      pauseBackgroundAudioDuringPlayback()
    }

    didFinishAutoplayEvent?()
  }

  private func addMediaObservers() {
    guard let duration else {
      return
    }

    let media25PercentTime = CMTimeMultiplyByFloat64(duration, multiplier: 0.25)
    media25TimeObserver = player?.addBoundaryTimeObserver(
      forTimes: [NSValue(time: media25PercentTime)],
      queue: nil
    ) {
      [weak self] in
      self?.removeMedia25Observer()
      self?.didPlay25PercentEvent?()
    }

    media100TimeObserver = player?.addBoundaryTimeObserver(
      forTimes: [NSValue(time: duration)],
      queue: nil
    ) {
      [weak self] in
      self?.removeMedia100Observer()
      self?.didFinishPlaybackEvent?()
    }
  }

  private func removeMedia25Observer() {
    if let media25TimeObserver {
      player?.removeTimeObserver(media25TimeObserver)
    }
    media25TimeObserver = nil
  }

  private func removeMedia100Observer() {
    if let media100TimeObserver {
      player?.removeTimeObserver(media100TimeObserver)
    }
    media100TimeObserver = nil
  }

  private func addAutoplayPausedObserver() {
    autoplayPausedObserver = player?.observe(\.timeControlStatus) {
      [weak self] (player, _) in
      if player.timeControlStatus == .paused {
        self?.finishAutoplayIfNeeded()
      }
    }
  }

  private func removeAutoplayPausedObserver() {
    autoplayPausedObserver?.invalidate()
    autoplayPausedObserver = nil
  }

  private func removeObservers() {
    removeAutoplayPausedObserver()
    removeMedia25Observer()
    removeMedia100Observer()
  }

  private func allowBackgroundAudioDuringAutoplay() {
    try? AVAudioSession.sharedInstance().setCategory(.playback, options: [.mixWithOthers])
    try? AVAudioSession.sharedInstance().setActive(true)
  }

  private func pauseBackgroundAudioDuringPlayback() {
    try? AVAudioSession.sharedInstance().setActive(false)
    try? AVAudioSession.sharedInstance().setCategory(
      .playback,
      mode: .default,
      policy: .default,
      options: []
    )
  }

  private func isPlaylistActive() -> Bool {
    return PlaylistCoordinator.shared.playlistController != nil
  }

  private func parseStopFrameFromFilename(filename: String) -> Double? {
    var stopFrame: Double?
    if let range = filename.range(of: "\\.KF\\d+\\.", options: .regularExpression) {
      let numberString = filename[range].replacingOccurrences(of: ".KF", with: "")
        .replacingOccurrences(of: ".", with: "")
      stopFrame = Double(numberString)
    }
    return stopFrame
  }
}
