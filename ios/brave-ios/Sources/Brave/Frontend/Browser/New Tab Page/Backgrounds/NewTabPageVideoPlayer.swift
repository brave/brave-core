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
  var autoplayFinishedEvent: ((Bool) -> Void)?
  var videoLoadedEvent: ((Bool) -> Void)?
  var playFinishedEvent: (() -> Void)?
  var playStartedEvent: (() -> Void)?
  var played25PercentEvent: (() -> Void)?
  var playCancelledEvent: ((Bool) -> Void)?

  var player: AVPlayer?

  private var playStarted = false
  private var previewAutoplayFinished = false
  private var isLoadFinished = false
  private var isAutoplayStartedOnce = false
  private var shouldStartAutoplayAfterLoad = false
  private var timeObserver: Any?
  private var playerObserver: NSKeyValueObservation?

  private let kMaxAutoplayDuration = 6.0

  private var stopFrame: Int?
  private var frameRate: Float?

  init(_ backgroundVideoPath: URL) {
    let asset: AVURLAsset = AVURLAsset(url: backgroundVideoPath)
    let item = AVPlayerItem(asset: asset)
    player = AVPlayer(playerItem: item)

    stopFrame = parseStopFrameFromFilename(filename: backgroundVideoPath.lastPathComponent)

    loadVideoTrackParams(
      asset: asset
    )

    NotificationCenter.default
      .addObserver(
        self,
        selector: #selector(self.playerDidFinishPlaying),
        name: .AVPlayerItemDidPlayToEndTime,
        object: item
      )
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
    if !previewAutoplayFinished {
      autoplayFinished(animated: false)
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
    playStarted = true

    playStartedEvent?()
  }

  func maybeCancelPlay(animated: Bool) {
    if !previewAutoplayFinished && isLoadFinished {
      autoplayFinished(animated: false)
      player?.pause()
    }

    if playStarted {
      player?.pause()
      if let timeObserver = timeObserver {
        player?.removeTimeObserver(timeObserver)
      }
      timeObserver = nil

      playCancelledEvent?(animated)
    }
    playStarted = false
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

  private func loadVideoTrackParams(asset: AVURLAsset) {
    Task {
      var loadedFrameRate: Float?
      let isPlayable = try? await asset.load(.isPlayable)
      if let isPlayable = isPlayable,
        isPlayable,
        let videoTrack = try? await asset.loadTracks(withMediaType: .video).first
      {
        loadedFrameRate = try? await videoTrack.load(.nominalFrameRate)
      }
      let frameRate = loadedFrameRate

      finishPlayerSetup(isPlayable: isPlayable, frameRate: frameRate)
    }
  }

  private func finishPlayerSetup(isPlayable: Bool?, frameRate: Float?) {
    DispatchQueue.main.async { [weak self] in
      guard let isPlayable = isPlayable else {
        self?.videoLoaded(succeeded: false)
        return
      }
      self?.videoLoaded(succeeded: isPlayable)
    }
  }

  func maybeStartAutoplay(shouldShowVideoBackground: Bool) {
    if isAutoplayStartedOnce {
      return
    }
    isAutoplayStartedOnce = true

    if !shouldShowVideoBackground {
      autoplayFinished()
      return
    }

    if previewAutoplayFinished {
      return
    }

    if !isLoadFinished {
      shouldStartAutoplayAfterLoad = true
      return
    }

    startAutoplay()
  }

  private func startAutoplay() {
    guard let duration = player?.currentItem?.duration else {
      autoplayFinished()
      return
    }

    var autoplayLengthSeconds: Float64 = self.kMaxAutoplayDuration
    if let frameRate = frameRate,
      let stopFrame = stopFrame
    {
      let stopFrameTime = Float64(stopFrame) / Float64(frameRate)
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
      if player.timeControlStatus == .paused && self?.previewAutoplayFinished == false {
        self?.autoplayFinished()
      }
    }

    player?.isMuted = true
    player?.play()
  }

  private func isVideoInProgress() -> Bool {
    return player?.timeControlStatus != .paused
  }

  @objc private func playerDidFinishPlaying(note: NSNotification) {
    if !previewAutoplayFinished {
      autoplayFinished()
      return
    }

    playFinishedEvent?()
    playStarted = false
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
    videoLoadedEvent?(succeeded)

    if !succeeded {
      autoplayFinished()
      return
    }

    if shouldStartAutoplayAfterLoad {
      startAutoplay()
    }
  }

  private func autoplayFinished(animated: Bool = true) {
    previewAutoplayFinished = true
    autoplayFinishedEvent?(animated)
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
