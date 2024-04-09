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
  var autoplayFinishedEvent: (() -> Void)?
  var autoplayStartedEvent: (() -> Void)?
  var playFinishedEvent: (() -> Void)?
  var playStartedEvent: (() -> Void)?
  var played25PercentEvent: (() -> Void)?
  var playCancelledEvent: (() -> Void)?

  var player: AVPlayer?

  private var isPlayStarted = false
  private var isLoadFinished = false
  private var isAutoplayRequestedOnce = false
  private var isAutoplayFinished = false
  private var timeObserver: Any?
  private var playerObserver: NSKeyValueObservation?

  private let kMaxAutoplayDuration = 6.0

  private var stopFrame: Int?
  private var frameRate: Float?

  init(backgroundVideoPath: URL, videoInitiallyVisible: Bool) {
    // Do not start autoplay if video is not initially visible
    if !videoInitiallyVisible {
      isAutoplayRequestedOnce = true
      isAutoplayFinished = true
    }

    stopFrame = parseStopFrameFromFilename(filename: backgroundVideoPath.lastPathComponent)
    resetPlayer(backgroundVideoPath)
    loadVideoTrackParams(backgroundVideoPath)
  }

  deinit {
    cleanupObservers()
  }

  func resetPlayer(_ backgroundVideoPath: URL?) {
    guard let backgroundVideoPath = backgroundVideoPath else {
      cleanupObservers()
      player = nil
      return
    }

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

  func startVideoPlayback() {
    if !isAutoplayFinished {
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
        self?.checkPlayPercentage()
      }

    player?.play()
    isPlayStarted = true

    playStartedEvent?()
  }

  func maybeCancelPlay() {
    if !isAutoplayFinished && isLoadFinished {
      autoplayFinished()
      player?.pause()
    }

    if isPlayStarted {
      player?.pause()
      if let timeObserver = timeObserver {
        player?.removeTimeObserver(timeObserver)
      }
      timeObserver = nil

      playCancelledEvent?()
    }
    isPlayStarted = false
  }

  func togglePlay() -> Bool {
    let isVideoInProgress = isVideoInProgress()

    if isVideoInProgress {
      player?.pause()
    } else {
      player?.play()
    }

    return !isVideoInProgress
  }

  func maybeStartAutoplay() {
    // Start autoplay only once.
    if isAutoplayRequestedOnce {
      return
    }
    isAutoplayRequestedOnce = true

    // Start autoplay after video is loaded
    if !isLoadFinished {
      return
    }

    startAutoplay()
  }

  private func loadVideoTrackParams(_ backgroundVideoPath: URL) {
    Task {
      var frameRate: Float?
      let asset: AVURLAsset = AVURLAsset(url: backgroundVideoPath)
      let isPlayable = try? await asset.load(.isPlayable)
      if let isPlayable = isPlayable,
        isPlayable,
        let videoTrack = try? await asset.loadTracks(withMediaType: .video).first
      {
        frameRate = try? await videoTrack.load(.nominalFrameRate)
      }

      finishPlayerSetup(isPlayable: isPlayable, frameRate: frameRate)
    }
  }

  private func finishPlayerSetup(isPlayable: Bool?, frameRate: Float?) {
    DispatchQueue.main.async { [weak self] in
      guard let self = self else { return }

      guard let isPlayable = isPlayable else {
        self.videoLoaded(succeeded: false)
        return
      }
      self.frameRate = frameRate
      self.videoLoaded(succeeded: isPlayable)
    }
  }

  private func startAutoplay() {
    if isAutoplayFinished {
      autoplayFinished()
      return
    }

    guard let duration = player?.currentItem?.duration else {
      autoplayFinished()
      return
    }

    var autoplayLengthSeconds = self.kMaxAutoplayDuration
    if let frameRate = frameRate,
      let stopFrame = stopFrame
    {
      let stopFrameTime = Double(stopFrame) / Double(frameRate)
      if stopFrameTime < autoplayLengthSeconds {
        autoplayLengthSeconds = stopFrameTime
      }
    }

    // If autoplay length is less then video duration then set forward playback end
    // time to stop the video at the specified time.
    if autoplayLengthSeconds < CMTimeGetSeconds(duration) {
      player?.currentItem?.forwardPlaybackEndTime = CMTime(
        seconds: autoplayLengthSeconds,
        preferredTimescale: CMTimeScale(NSEC_PER_SEC)
      )
    }

    // Detect if video was interrupted during autoplay and fire autoplayFinished
    // event if needed.
    playerObserver = player?.observe(\.timeControlStatus, options: [.new, .old]) {
      [weak self] (player, change) in
      guard let self = self else { return }
      if player.timeControlStatus == .paused && !self.isAutoplayFinished {
        self.autoplayFinished()
      }
    }

    player?.isMuted = true
    player?.play()

    autoplayStartedEvent?()
  }

  private func isVideoInProgress() -> Bool {
    return player?.timeControlStatus != .paused
  }

  @objc private func playerDidFinishPlaying(note: NSNotification) {
    if !isAutoplayFinished {
      autoplayFinished()
      return
    }

    playFinishedEvent?()
    isPlayStarted = false
  }

  private func checkPlayPercentage() {
    guard let timeObserver = timeObserver else {
      return
    }

    guard let playerItem = player?.currentItem else {
      return
    }

    let duration = CMTimeGetSeconds(playerItem.duration)
    let currentTime = CMTimeGetSeconds(playerItem.currentTime())

    if currentTime / duration >= 0.25 {
      played25PercentEvent?()

      player?.removeTimeObserver(timeObserver)
      self.timeObserver = nil
    }
  }

  private func videoLoaded(succeeded: Bool) {
    isLoadFinished = true

    if !succeeded {
      autoplayFinished()
      return
    }

    // Autoplay didn't start before because the video wasn't loaded
    if isAutoplayRequestedOnce {
      startAutoplay()
    }
  }

  private func autoplayFinished() {
    isAutoplayFinished = true
    autoplayFinishedEvent?()
  }

  private func cleanupObservers() {
    NotificationCenter.default.removeObserver(self)
    if let timeObserver = timeObserver {
      player?.removeTimeObserver(timeObserver)
    }
    timeObserver = nil
    playerObserver?.invalidate()
  }

  private func parseStopFrameFromFilename(filename: String) -> Int? {
    var stopFrame: Int?
    if let range = filename.range(of: "\\.KF\\d+\\.", options: .regularExpression) {
      let numberString = filename[range].replacingOccurrences(of: ".KF", with: "")
        .replacingOccurrences(of: ".", with: "")
      stopFrame = Int(numberString)
    }
    return stopFrame
  }
}
