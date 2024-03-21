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

class NewTabPageVideoBackgroundController: UIView {
  var playFinishedEvent: (() -> Void)?
  var played25PercentEvent: (() -> Void)?
  var previewPlayFinishedEvent: (() -> Void)?
  var playStartedEvent: (() -> Void)?
  var playPausedEvent: (() -> Void)?
  var playStarted: Bool?

  private var playerLayer: AVPlayerLayer?
  private var previewAutoplayFinished: Bool = false
  private var timeObserverToken: Any?
  private var isShortVideo = false

  private let kMaxAutoplayDuration = 6.0

  override init(frame: CGRect) {
    super.init(frame: frame)
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  func setupPlayerAndStartAutoplay(backgroundVideoPath: URL, backgroundColor: String?) {
    self.backgroundColor = parseColorFromString(color: backgroundColor)
    // let documentsURL = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
    // let videoURL = documentsURL.appendingPathComponent("Downloads/result/car-KF100").appendingPathExtension("mp4")

    var autoplayLengthSeconds: Float64 = kMaxAutoplayDuration

    let asset: AVAsset = AVAsset(url: backgroundVideoPath)
    isShortVideo = false

    let stopFrame = parseStopFrameFromFilename(filename: backgroundVideoPath.lastPathComponent)

    Task {
      if let videoTrack = try? await asset.loadTracks(withMediaType: .video).first {
        let frameRate = try? await videoTrack.load(.nominalFrameRate)
        let duration = try? await asset.load(.duration)

        guard let duration = duration else {
          return
        }

        isShortVideo = CMTimeGetSeconds(duration) <= kMaxAutoplayDuration

        if isShortVideo {
          autoplayLengthSeconds = CMTimeGetSeconds(duration)
        } else if let frameRate = frameRate,
          let stopFrame = stopFrame
        {
          autoplayLengthSeconds = Float64(stopFrame) / Float64(frameRate)
        }
      }

      let item = AVPlayerItem(asset: asset)
      let player = AVPlayer(playerItem: item)

      var newPlayerLayer = AVPlayerLayer(player: player)
      newPlayerLayer.frame = self.bounds

      if let playerLayer = self.playerLayer {
        self.layer.replaceSublayer(playerLayer, with: newPlayerLayer)
      } else {
        self.layer.addSublayer(newPlayerLayer)
      }
      self.playerLayer = newPlayerLayer

      startPreview(autoplayLengthSeconds: autoplayLengthSeconds)
    }
  }

  private func startPreview(autoplayLengthSeconds: Float64) {
    if !isShortVideo {
      playerLayer?.player?.currentItem?.forwardPlaybackEndTime = CMTime(
        seconds: autoplayLengthSeconds,
        preferredTimescale: CMTimeScale(NSEC_PER_SEC)
      )
      NotificationCenter.default.addObserver(
        self,
        selector: #selector(playerItemTimeJumped(_:)),
        name: .AVPlayerItemTimeJumped,
        object: playerLayer?.player?.currentItem
      )
    }

    NotificationCenter.default
      .addObserver(
        self,
        selector: #selector(self.playerDidFinishPlaying),
        name: .AVPlayerItemDidPlayToEndTime,
        object: playerLayer?.player?.currentItem
      )

    playerLayer?.player?.isMuted = true
    playerLayer?.player?.play()
  }

  @objc func playerItemTimeJumped(_ notification: Notification) {
    if isShortVideo || previewAutoplayFinished {
      return
    }
    guard let playerItem = notification.object as? AVPlayerItem else {
      return
    }

    let currentTime = playerItem.currentTime().seconds

    let forwardEndTime = playerItem.forwardPlaybackEndTime.seconds
    if currentTime >= forwardEndTime {
      previewAutoplayFinished = true
      previewPlayFinishedEvent?()
    }
  }

  @objc private func playerDidFinishPlaying(note: NSNotification) {
    if !previewAutoplayFinished && isShortVideo {
      previewAutoplayFinished = true
      previewPlayFinishedEvent?()
      return
    }

    if playStarted == nil {
      return
    }
    playStarted = nil

    playFinishedEvent?()
  }

  private func checkPlayPercentage() {
    guard let playerLayer = playerLayer else {
      return
    }

    if timeObserverToken == nil {
      return
    }

    let playerItem = playerLayer.player?.currentItem!
    let duration = CMTimeGetSeconds(playerItem!.duration)
    let currentTime = CMTimeGetSeconds(playerItem!.currentTime())

    if currentTime / duration >= 0.25 {
      played25PercentEvent?()

      playerLayer.player?.removeTimeObserver(timeObserverToken!)
      timeObserverToken = nil
    }
  }

  func playOrPauseNTTVideo() {
    guard let playerLayer = playerLayer else {
      return
    }

    if playStarted == nil {
      if !previewAutoplayFinished {
        previewPlayFinishedEvent?()
        previewAutoplayFinished = true
      }

      playerLayer.player?.isMuted = false
      playerLayer.player?.pause()
      playerLayer.player?.seek(to: CMTime.zero)
      playerLayer.player?.currentItem?.forwardPlaybackEndTime = CMTime()

      let interval = CMTime(
        seconds: 0.1,
        preferredTimescale: CMTimeScale(NSEC_PER_SEC)
      )
      let mainQueue = DispatchQueue.main
      timeObserverToken =
        playerLayer.player?.addPeriodicTimeObserver(forInterval: interval, queue: mainQueue) {
          [weak self] time in
          self?.checkPlayPercentage()
        }

      playStarted = false
    }

    if !playStarted! {
      playStartedEvent?()

      playerLayer.player?.play()
      playStarted = true
    } else {
      playPausedEvent?()

      playerLayer.player?.pause()
      playStarted = false
    }
  }

  private func parseColorFromString(color: String?) -> UIColor {
    guard var color = color?.trimmingCharacters(in: .whitespacesAndNewlines) else {
      return UIColor.black
    }

    if !color.hasPrefix("#") && color.count != 7 {
      return UIColor.black
    }

    color.remove(at: color.startIndex)

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
    if let range = filename.range(of: "-KF(\\d+)\\.mp4", options: .regularExpression) {
      let numberString = filename[range].replacingOccurrences(of: "-KF", with: "")
        .replacingOccurrences(of: ".mp4", with: "")
      stopFrame = Int(numberString)
    }
    return stopFrame
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
