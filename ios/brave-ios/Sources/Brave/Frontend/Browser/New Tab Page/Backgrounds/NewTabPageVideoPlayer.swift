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

/// New Tab Page video player?. The class is responsible for managing the video
/// playback on the New Tab Page. It handles events such as play finished,
/// played 25 percent, autoplay finished, play cancelled, and video loaded.
class NewTabPageVideoPlayer {
  var didStartAutoplayEvent: (() -> Void)?
  var didFinishAutoplayEvent: (() -> Void)?
  var didStartPlaybackEvent: (() -> Void)?
  var didFinishPlaybackEvent: (() -> Void)?
  var didPlay25PercentEvent: (() -> Void)?
  var didCancelPlaybackEvent: (() -> Void)?

  var player: AVPlayer?

  private let backgroundVideoPath: URL
  private var didStartPlayback = false
  private var didStartAutoplay = false
  private var didFinishAutoplay = false
  private var timeObserver: Any?
  private var playerObserver: NSKeyValueObservation?

  private let kMaxAutoplayDurationInSeconds = 6.0
  private let kMedia25Percent = 0.25
  private let kStopFramePositionAdjustment = 0.5

  private var duration: CMTime?
  private var frameRate: Double?

  private var isPlaying: Bool {
    player?.timeControlStatus != .paused
  }

  init(_ backgroundVideoPath: URL) {
    self.backgroundVideoPath = backgroundVideoPath
    createPlayer()
  }

  deinit {
    cleanupObservers()
  }

  func resetPlayer() {
    cleanupObservers()
    player = nil
  }

  func createPlayer() {
    let item = AVPlayerItem(url: backgroundVideoPath)
    player = AVPlayer(playerItem: item)

    NotificationCenter.default
      .addObserver(
        self,
        selector: #selector(self.playerDidFinishPlaying),
        name: .AVPlayerItemDidPlayToEndTime,
        object: item
      )
  }

  func startPlayback() {
    if !didFinishAutoplay {
      autoplayFinished()
    }

    player?.isMuted = false
    player?.seek(to: CMTime.zero)
    player?.currentItem?.forwardPlaybackEndTime = CMTime()

    let interval = CMTime(
      seconds: 0.1,
      preferredTimescale: CMTimeScale(NSEC_PER_SEC)
    )
    // Setup observer to check if 25% of the video is played.
    timeObserver =
      player?.addPeriodicTimeObserver(forInterval: interval, queue: nil) {
        [weak self] time in
        self?.checkMedia25Percentage()
      }

    player?.play()
    didStartPlayback = true

    didStartPlaybackEvent?()
  }

  func cancelAutoplay() {
    didStartAutoplay = true
  }

  func seekToStopFrame() {
    player?.seek(to: stopFrameTime(), toleranceBefore: CMTime.zero, toleranceAfter: CMTime.zero)
  }

  func maybeCancelPlay() {
    if !didFinishAutoplay {
      autoplayFinished()
      player?.pause()
      seekToStopFrame()
    } else if didStartPlayback {
      player?.pause()
      if let timeObserver = timeObserver {
        player?.removeTimeObserver(timeObserver)
      }
      timeObserver = nil

      didCancelPlaybackEvent?()
      didStartPlayback = false
    }
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

  func maybeStartAutoplay() {
    if !didStartAutoplay {
      loadTrackParams(backgroundVideoPath)
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

  private func loadTrackParams(_ backgroundVideoPath: URL) {
    Task {
      do {
        var frameRate: Float?
        let asset: AVURLAsset = AVURLAsset(url: backgroundVideoPath)
        let (isPlayable, duration) = try await asset.load(.isPlayable, .duration)
        if isPlayable {
          if let track = try? await asset.loadTracks(withMediaType: .video).first {
            frameRate = try? await track.load(.nominalFrameRate)
          }
          await loadedTrackParams(duration: duration, frameRate: frameRate)
        }
      } catch {}
    }
  }

  @MainActor
  private func loadedTrackParams(duration: CMTime?, frameRate: Float?) async {
    self.duration = duration
    if let frameRate {
      self.frameRate = Double(frameRate)
    }

    startAutoplay()
  }

  private func startAutoplay() {
    cancelAutoplay()

    guard let duration else {
      autoplayFinished()
      return
    }

    let forwardPlaybackEndTime = stopFrameTime()
    // If autoplay length is less then video duration then set forward playback
    // end time to stop the video at the specified time.
    if forwardPlaybackEndTime < duration {
      player?.currentItem?.forwardPlaybackEndTime = forwardPlaybackEndTime
    }

    // Detect if video was interrupted during autoplay and fire
    // `autoplayFinished` event if needed.
    playerObserver = player?.observe(\.timeControlStatus) {
      [weak self] (player, _) in
      guard let self = self else { return }
      if player.timeControlStatus == .paused && !self.didFinishAutoplay {
        self.autoplayFinished()
        self.seekToStopFrame()
      }
    }

    player?.isMuted = true
    player?.play()

    didStartAutoplayEvent?()
  }

  @objc private func playerDidFinishPlaying() {
    if !didFinishAutoplay {
      autoplayFinished()
      return
    }

    didFinishPlaybackEvent?()
    didStartPlayback = false
  }

  private func checkMedia25Percentage() {
    guard let timeObserver, let duration, let playerItem = player?.currentItem else {
      return
    }

    let durationInSeconds = CMTimeGetSeconds(duration)
    let currentTime = CMTimeGetSeconds(playerItem.currentTime())

    if currentTime / durationInSeconds >= kMedia25Percent {
      didPlay25PercentEvent?()

      player?.removeTimeObserver(timeObserver)
      self.timeObserver = nil
    }
  }

  private func autoplayFinished() {
    didFinishAutoplay = true
    didFinishAutoplayEvent?()
  }

  private func cleanupObservers() {
    NotificationCenter.default.removeObserver(self)
    if let timeObserver = timeObserver {
      player?.removeTimeObserver(timeObserver)
    }
    timeObserver = nil
    playerObserver?.invalidate()
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
