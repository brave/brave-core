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

/// New Tab Page video player. The class is responsible for managing the video
/// playback on the New Tab Page. It handles events such as play finished,
/// played 25 percent, autoplay finished, play cancelled, and video loaded.
class NewTabPageVideoBackgroundController: UIViewController {
  var autoplayFinishedEvent: ((Bool) -> Void)?
  var playCancelledEvent: ((Bool) -> Void)?
  var videoLoadedEvent: ((Bool) -> Void)?
  var playFinishedEvent: (() -> Void)?
  var playStartedEvent: (() -> Void)?
  var played25PercentEvent: (() -> Void)?

  private let kMaxAutoplayDuration = 6.0

  private let background: NewTabPageBackground
  private var playerLayer = AVPlayerLayer()
  private var playStarted = false
  private var previewAutoplayFinished: Bool = false
  private var timeObserver: Any?
  private var playerObserver: NSKeyValueObservation?

  private var videoButtonsView = NewTabPageVideoButtonsView()

  init(background: NewTabPageBackground) {
    self.background = background
    super.init(nibName: nil, bundle: nil)
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
    if let timeObserver = timeObserver {
      playerLayer.player?.removeTimeObserver(timeObserver)
    }
    timeObserver = nil
    playerObserver?.invalidate()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    playerLayer.frame = view.frame
    view.layer.addSublayer(playerLayer)

    // Show and setup the video player if only there is background video.
    view.isHidden = background.backgroundVideoPath == nil
    if let backgroundVideoPath = background.backgroundVideoPath {
      setupPlayer(backgroundVideoPath: backgroundVideoPath)
    }

    view.addSubview(videoButtonsView)
    videoButtonsView.isHidden = true

    videoButtonsView.tappedBackground = { [weak self] in
      guard let isVideoInProgress = self?.isVideoInProgress() else {
        return false
      }

      if isVideoInProgress {
        self?.playerLayer.player?.pause()
      } else {
        self?.playerLayer.player?.play()
      }

      return !isVideoInProgress
    }
    videoButtonsView.tappedCancelButton = { [weak self] in
      self?.cancelPlay(animated: true)
    }

    videoButtonsView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    self.playerLayer.frame = self.view.bounds

    updatePlayerVisibility()
  }

  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)

    // Cancel play if view becomes hidden.
    cancelPlay(animated: false)
  }

  func startVideoPlayback() {
    if !previewAutoplayFinished {
      autoplayFinished()
    }

    self.videoButtonsView.isHidden = false

    playerLayer.player?.isMuted = false
    playerLayer.player?.seek(to: CMTime.zero)
    playerLayer.player?.currentItem?.forwardPlaybackEndTime = CMTime()

    let interval = CMTime(
      seconds: 0.1,
      preferredTimescale: CMTimeScale(NSEC_PER_SEC)
    )
    // Setup observer to check if 25% of the video is played.
    timeObserver =
      playerLayer.player?.addPeriodicTimeObserver(forInterval: interval, queue: nil) {
        [weak self] time in
        self?.checkPlayPercentage()
      }

    playerLayer.player?.play()
    playStarted = true

    playStartedEvent?()
  }

  private func setupPlayer(backgroundVideoPath: URL) {
    self.view.backgroundColor = parseColorFromFilename(
      filename: backgroundVideoPath.lastPathComponent
    )
    let stopFrame = parseStopFrameFromFilename(filename: backgroundVideoPath.lastPathComponent)
    let resizeToFill = shouldResizeToFill(filename: backgroundVideoPath.lastPathComponent)

    let asset: AVURLAsset = AVURLAsset(url: backgroundVideoPath)
    loadVideoTrackParams(
      asset: asset,
      stopFrame: stopFrame,
      resizeToFill: resizeToFill
    )
  }

  private func loadVideoTrackParams(asset: AVURLAsset, stopFrame: Int?, resizeToFill: Bool) {
    Task {
      let isPlayable = try? await asset.load(.isPlayable)
      let duration = try? await asset.load(.duration)
      var frameRate: Float?
      if let videoTrack = try? await asset.loadTracks(withMediaType: .video).first {
        frameRate = try? await videoTrack.load(.nominalFrameRate)
      }

      guard let duration = duration,
        let isPlayable = isPlayable
      else {
        videoLoaded(succeeded: false)
        return
      }
      videoLoaded(succeeded: isPlayable)

      setupPlayerLayer(
        asset: asset,
        resizeToFill: resizeToFill,
        duration: duration,
        frameRate: frameRate,
        stopFrame: stopFrame
      )
    }
  }

  private func setupPlayerLayer(
    asset: AVURLAsset,
    resizeToFill: Bool,
    duration: CMTime,
    frameRate: Float?,
    stopFrame: Int?
  ) {
    let item = AVPlayerItem(asset: asset)
    let player = AVPlayer(playerItem: item)
    playerLayer.player = player

    if resizeToFill {
      playerLayer.videoGravity = .resizeAspectFill
    } else {
      playerLayer.videoGravity = .resizeAspect
    }

    NotificationCenter.default
      .addObserver(
        self,
        selector: #selector(self.playerDidFinishPlaying),
        name: .AVPlayerItemDidPlayToEndTime,
        object: playerLayer.player?.currentItem
      )

    // Do not start autoplay in landscape mode on Phone.
    if shouldShowVideoBackground() {
      startAutoplay(duration: duration, frameRate: frameRate, stopFrame: stopFrame)
    } else {
      autoplayFinished()
    }
  }

  private func startAutoplay(duration: CMTime, frameRate: Float?, stopFrame: Int?) {
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
      playerLayer.player?.currentItem?.forwardPlaybackEndTime = CMTime(
        seconds: autoplayLengthSeconds,
        preferredTimescale: CMTimeScale(NSEC_PER_SEC)
      )
    }

    // Detect if video was interrupted during autoplay and fire autoplayFinished
    // event if needed.
    playerObserver = playerLayer.player?.observe(\.timeControlStatus, options: [.new, .old]) {
      [weak self] (player, change) in
      if player.timeControlStatus == .paused && self?.previewAutoplayFinished == false {
        self?.autoplayFinished()
      }
    }

    playerLayer.player?.isMuted = true
    playerLayer.player?.play()
  }

  private func isVideoInProgress() -> Bool {
    return playerLayer.player?.timeControlStatus != .paused
  }

  private func updatePlayerVisibility() {
    if shouldShowVideoBackground() {
      self.playerLayer.isHidden = false
    } else {
      // Cancel play and hide the player layer in landscape mode on Phone.
      self.playerLayer.isHidden = true
      cancelPlay(animated: false)
    }
  }

  private func cancelPlay(animated: Bool) {
    if !previewAutoplayFinished && playerLayer.player != nil {
      autoplayFinished(animated: false)
      playerLayer.player?.pause()
    }

    if playStarted {
      playerLayer.player?.pause()
      if let timeObserver = timeObserver {
        playerLayer.player?.removeTimeObserver(timeObserver)
      }
      timeObserver = nil

      videoButtonsView.isHidden = true
      playCancelledEvent?(animated)
    }
    playStarted = false
  }

  @objc private func playerDidFinishPlaying(note: NSNotification) {
    if !previewAutoplayFinished {
      autoplayFinished()
      return
    }

    self.videoButtonsView.isHidden = true

    playFinishedEvent?()
    playStarted = false
  }

  private func checkPlayPercentage() {
    guard let timeObserver = timeObserver else {
      return
    }

    guard let playerItem = playerLayer.player?.currentItem else {
      return
    }

    let duration = CMTimeGetSeconds(playerItem.duration)
    let currentTime = CMTimeGetSeconds(playerItem.currentTime())

    if currentTime / duration >= 0.25 {
      played25PercentEvent?()

      playerLayer.player?.removeTimeObserver(timeObserver)
      self.timeObserver = nil
    }
  }

  private func shouldShowVideoBackground() -> Bool {
    let isLandscape = view.frame.width > view.frame.height
    let isPhone = UIDevice.isPhone
    return !(isLandscape && isPhone)
  }

  private func videoLoaded(succeeded: Bool) {
    videoLoadedEvent?(succeeded)
    if !succeeded {
      autoplayFinished()
    }
  }

  private func autoplayFinished(animated: Bool = true) {
    previewAutoplayFinished = true
    autoplayFinishedEvent?(animated)
  }

  private func parseColorFromFilename(filename: String) -> UIColor {
    var color: String?
    if let range = filename.range(of: "\\.RGB[a-fA-F0-9]+\\.", options: .regularExpression) {
      color = filename[range].replacingOccurrences(of: ".RGB", with: "")
        .replacingOccurrences(of: ".", with: "")
    }

    guard let color = color,
      color.count == 6
    else {
      return UIColor.black
    }

    var rgbValue: UInt64 = 0
    Scanner(string: color).scanHexInt64(&rgbValue)

    return UIColor(
      red: CGFloat((rgbValue & 0xFF0000) >> 16) / 255.0,
      green: CGFloat((rgbValue & 0x00FF00) >> 8) / 255.0,
      blue: CGFloat(rgbValue & 0x0000FF) / 255.0,
      alpha: CGFloat(1.0)
    )
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

  private func shouldResizeToFill(filename: String) -> Bool {
    return filename.range(of: "\\.RTF\\.", options: .regularExpression) != nil
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
