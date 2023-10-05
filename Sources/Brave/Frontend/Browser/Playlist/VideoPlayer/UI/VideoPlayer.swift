// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveShared
import Shared
import AVKit
import AVFoundation
import Playlist
import MediaPlayer

protocol VideoViewDelegate: AnyObject {
  func onPreviousTrack(_ videoView: VideoView, isUserInitiated: Bool)
  func onNextTrack(_ videoView: VideoView, isUserInitiated: Bool)
  func onSidePanelStateChanged(_ videoView: VideoView)
  func onPictureInPicture(_ videoView: VideoView)
  func onFullscreen(_ videoView: VideoView)
  func onExitFullscreen(_ videoView: VideoView)
  func onFavIconSelected(_ videoView: VideoView)

  func play(_ videoView: VideoView)
  func pause(_ videoView: VideoView)
  func stop(_ videoView: VideoView)
  func seekBackwards(_ videoView: VideoView)
  func seekForwards(_ videoView: VideoView)
  func seek(_ videoView: VideoView, to time: TimeInterval)
  func seek(_ videoView: VideoView, relativeOffset: Float)
  func setPlaybackRate(_ videoView: VideoView, rate: Float)
  func togglePlayerGravity(_ videoView: VideoView)
  func toggleRepeatMode(_ videoView: VideoView)

  var isPlaying: Bool { get }
  var repeatMode: MediaPlayer.RepeatMode { get }
  var playbackRate: Float { get }
  var isVideoTracksAvailable: Bool { get }
}

class VideoView: UIView, VideoTrackerBarDelegate {

  weak var delegate: VideoViewDelegate? {
    didSet {
      self.toggleOverlays(showOverlay: true)
    }
  }

  private let particleView = PlaylistParticleEmitter().then {
    $0.alpha = 0.0
    $0.contentMode = .scaleAspectFit
    $0.clipsToBounds = true
  }

  private let overlayView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.isUserInteractionEnabled = true
    $0.backgroundColor = #colorLiteral(red: 0, green: 0, blue: 0, alpha: 0.4024561216)
  }
  
  private let staticImageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.isUserInteractionEnabled = true
    $0.backgroundColor = .black
  }

  let infoView = VideoPlayerInfoBar().then {
    $0.layer.cornerRadius = 18.0
    $0.layer.masksToBounds = true
  }

  let controlsView = VideoPlayerControlsView().then {
    $0.layer.cornerRadius = 18.0
    $0.layer.masksToBounds = true
  }

  // State
  var isOverlayDisplayed = false

  private var isMediaLive = false
  private var isSeeking = false
  private(set) var isFullscreen = false
  private var wasPlayingBeforeSeeking = false
  private var playbackRate: Float = 1.0
  private var playerLayer: AVPlayerLayer?
  private var fadeAnimationWorkItem: DispatchWorkItem?
  private var playerStatusObserver: NSObjectProtocol?

  // Vars and enums for handling gestures on the playerLayer (changing volume, scrolling the timeline)
  private enum DraggingDirection {
    case noDirection
    case verticalDirection
    case horizontalDirection
  }

  private var currentDraggingDirection: DraggingDirection = .noDirection
  private var dragStartedTimelinePos = 0.0
  private var dragStartedVolume: Float = 0.0

  private let volumeView = MPVolumeView()
  private var seekGestures = [UIGestureRecognizer]()

  override init(frame: CGRect) {
    super.init(frame: frame)

    // Setup
    backgroundColor = .black

    // Controls
    infoView.sidePanelButton.addTarget(self, action: #selector(onSidePanel(_:)), for: .touchUpInside)
    infoView.pictureInPictureButton.addTarget(self, action: #selector(onPictureInPicture(_:)), for: .touchUpInside)
    infoView.fullscreenButton.addTarget(self, action: #selector(onFullscreen(_:)), for: .touchUpInside)
    infoView.exitButton.addTarget(self, action: #selector(onExitFullscreen(_:)), for: .touchUpInside)

    controlsView.repeatButton.addTarget(self, action: #selector(onRepeat(_:)), for: .touchUpInside)
    controlsView.playPauseButton.addTarget(self, action: #selector(onPlay(_:)), for: .touchUpInside)
    controlsView.playbackRateButton.addTarget(self, action: #selector(onPlaybackRateChanged(_:)), for: .touchUpInside)
    controlsView.skipBackButton.addTarget(self, action: #selector(onSeekBackwards(_:)), for: .touchUpInside)
    controlsView.skipForwardButton.addTarget(self, action: #selector(onSeekForwards(_:)), for: .touchUpInside)
    controlsView.skipBackButton.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(onSeekPrevious(_:))))
    controlsView.skipForwardButton.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(onSeekNext(_:))))
    controlsView.nextButton.addTarget(self, action: #selector(onNextTrack(_:)), for: .touchUpInside)

    // Layout
    addSubview(particleView)
    addSubview(overlayView)
    addSubview(infoView)
    addSubview(controlsView)
    addSubview(staticImageView)

    particleView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    overlayView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    infoView.snp.makeConstraints {
      $0.leading.equalTo(self.safeArea.leading).inset(8.0)
      $0.trailing.equalTo(self.safeArea.trailing).inset(8.0)
      $0.top.equalTo(self.safeArea.top).inset(8.0)
    }

    controlsView.snp.makeConstraints {
      $0.leading.equalTo(self.safeArea.leading).inset(8.0)
      $0.trailing.equalTo(self.safeArea.trailing).inset(8.0)
      $0.bottom.equalTo(self.safeArea.bottom).inset(8.0)
      $0.height.equalTo(100.0)
    }

    // Delegates
    controlsView.trackBar.delegate = self

    // Gestures
    let overlayTappedGesture = UITapGestureRecognizer(target: self, action: #selector(onOverlayTapped(_:))).then {
      $0.numberOfTapsRequired = 1
      $0.numberOfTouchesRequired = 1
    }

    let overlayDoubleTappedGesture = UITapGestureRecognizer(target: self, action: #selector(onOverlayDoubleTapped(_:))).then {
      $0.numberOfTapsRequired = 2
      $0.numberOfTouchesRequired = 1
      $0.delegate = self
    }

    // Used for timeline and volume gestures
    let overlayDraggedGesture = UIPanGestureRecognizer(target: self, action: #selector(onOverlayDragged(_:))).then {
      $0.delegate = self
    }

    let favIconTappedGesture = UITapGestureRecognizer(target: self, action: #selector(onFavIconTapped(_:))).then {
      $0.numberOfTapsRequired = 1
      $0.numberOfTouchesRequired = 1
    }

    seekGestures = [overlayDoubleTappedGesture, overlayDraggedGesture]
    addGestureRecognizer(overlayTappedGesture)
    addGestureRecognizer(overlayDoubleTappedGesture)
    addGestureRecognizer(overlayDraggedGesture)
    overlayTappedGesture.require(toFail: overlayDoubleTappedGesture)
    infoView.favIconImageView.addGestureRecognizer(favIconTappedGesture)

    // Logic
    self.toggleOverlays(showOverlay: true)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    playerLayer?.frame = self.bounds
    staticImageView.layer.frame = self.bounds
  }

  @objc
  private func onOverlayTapped(_ gestureRecognizer: UITapGestureRecognizer) {
    let isPlaying = delegate?.isPlaying == true

    if isSeeking {
      toggleOverlays(showOverlay: true, except: [overlayView, infoView, controlsView.playPauseButton], display: [controlsView.trackBar])
    } else if (isPlaying && !isOverlayDisplayed) || (!isPlaying && !isSeeking && !isOverlayDisplayed) {
      toggleOverlays(showOverlay: true)
      isOverlayDisplayed = true

      fadeAnimationWorkItem?.cancel()
      fadeAnimationWorkItem = DispatchWorkItem(block: { [weak self] in
        guard let self = self else { return }
        self.isOverlayDisplayed = false

        if self.delegate?.isPlaying == true && !self.isSeeking {
          self.toggleOverlays(showOverlay: false)
        }
      })

      guard let fadeAnimationWorkItem = fadeAnimationWorkItem else { return }
      DispatchQueue.main.asyncAfter(deadline: .now() + 5.0, execute: fadeAnimationWorkItem)
    } else if isPlaying && isOverlayDisplayed {
      toggleOverlays(showOverlay: false)
      isOverlayDisplayed = false
    } else {
      toggleOverlays(showOverlay: true)
      isOverlayDisplayed = true
    }
  }

  @objc
  private func onOverlayDoubleTapped(_ gestureRecognizer: UITapGestureRecognizer) {
    // Originally double tap changed size of the video to fit the screen,
    // now we will use this gesture to advance/go back by 15 secs.
    // Perhaps we should add a button to the player's toolbar overlay for toggling gravity.
    // delegate?.togglePlayerGravity(self)

    // Advance or go back by 15 seconds
    let tapLocation = gestureRecognizer.location(in: self)

    // If we got a tap in 0width...0.4width (left part of the view).
    if tapLocation.x > 0 && tapLocation.x < 0.4 * bounds.size.width {
      // Retreat by 15 seconds.
      self.seekBackwards()
      // If we got a tap in 0.6width...1width (right part of the view).
    } else if tapLocation.x > 0.6 * bounds.size.width && tapLocation.x < bounds.size.width {
      // Advance video by 15 seconds.
      self.seekForwards()
      // If there's a click in the central area
    } else if tapLocation.x > 0.4 * bounds.size.width && 0.4 * bounds.size.width < 0.6 * bounds.size.width {
      // Pause/resume the video
      if delegate?.isPlaying == true {
        self.pause()
      } else {
        self.play()
      }
    }
  }

  // This is where we know when panning gesture started and finished.
  override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
    if event?.type == .touches {
      dragStartedVolume = AVAudioSession.sharedInstance().outputVolume

      toggleOverlays(showOverlay: true)
      if let currentPlayer = playerLayer?.player {
        let currentSeconds = CGFloat(currentPlayer.currentTime().value) / CGFloat((currentPlayer.currentTime().timescale))
        dragStartedTimelinePos = currentSeconds
      }
    }

    super.touchesBegan(touches, with: event)
  }

  override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
    if event?.type == UIEvent.EventType.touches {
      toggleOverlays(showOverlay: false)
    }

    super.touchesEnded(touches, with: event)
  }

  @objc
  private func onOverlayDragged(_ gestureRecognizer: UIPanGestureRecognizer) {
    if gestureRecognizer.state == .ended {
      // User ended panning
      currentDraggingDirection = .noDirection

      guard let delegate = delegate else { return }

      isSeeking = false

      if wasPlayingBeforeSeeking {
        delegate.play(self)
        delegate.setPlaybackRate(self, rate: playbackRate)
        wasPlayingBeforeSeeking = false
      }

      if delegate.isPlaying {
        fadeAnimationWorkItem?.cancel()
        toggleOverlays(showOverlay: false, except: [overlayView], display: [overlayView])
        overlayView.alpha = 0.0
        isOverlayDisplayed = false
      } else {
        fadeAnimationWorkItem?.cancel()
        toggleOverlays(showOverlay: true)
        overlayView.alpha = 1.0
        isOverlayDisplayed = true
      }

    } else {
      if let currentPlayer = playerLayer?.player {

        // Horizontal or vertical pan?
        if currentDraggingDirection == .noDirection {
          if abs(gestureRecognizer.velocity(in: self).y) > abs(gestureRecognizer.velocity(in: self).x) {
            currentDraggingDirection = .verticalDirection
          } else {
            currentDraggingDirection = .horizontalDirection
          }
        }

        if currentDraggingDirection == .verticalDirection {

          // Vertical dragging adjusts volume.
          let vDragRatio = 4 * gestureRecognizer.translation(in: self).y / bounds.height  // (-0.5 ... 0.5) * 4

          if vDragRatio != 0 && self.dragStartedVolume >= 0 {
            if let slider = volumeView.subviews.first(where: { $0 is UISlider }) as? UISlider {
              let finalVal = self.dragStartedVolume - Float(vDragRatio)

              // Making sure finalVal is within 0..1 interval
              slider.value = min(max(0.0, finalVal), 1.0)
            }
          }
          // End of vertical panning processing.
        } else {

          // Otherwise it's a horizontal pan.
          let dragRatio = gestureRecognizer.translation(in: self).x / bounds.width  // (-0.5 ... 0.5)*1
          var vidDurationSeconds = 0.0
          if let currentItem = currentPlayer.currentItem {
            vidDurationSeconds = CGFloat((currentItem.duration.value)) / CGFloat((currentItem.duration.timescale))
          }
          var finalSeconds = dragStartedTimelinePos + dragRatio * vidDurationSeconds

          // Making sure we're not out of bounds
          finalSeconds = min(max(0.0, finalSeconds), vidDurationSeconds)

          guard let delegate = delegate else { return }

          isSeeking = true

          if delegate.isPlaying {
            wasPlayingBeforeSeeking = true
            playbackRate = delegate.playbackRate
            delegate.pause(self)
          }

          toggleOverlays(showOverlay: false, except: [infoView, controlsView], display: [controlsView])
          isOverlayDisplayed = true

          if let currentItem = currentPlayer.currentItem {
            let trackbarPos = CMTime(seconds: finalSeconds, preferredTimescale: currentItem.duration.timescale)
            let trackbarLen = CMTime(seconds: CGFloat((currentItem.duration.value)) / CGFloat((currentItem.duration.timescale)), preferredTimescale: currentItem.duration.timescale)

            controlsView.trackBar.setTimeRange(currentTime: trackbarPos, endTime: trackbarLen)  // Move the slider.
            seek(to: finalSeconds)  // And rewind the video.
          }
          // End of horizontal panning processing.
        }
      }
    }
  }

  @objc
  private func onFavIconTapped(_ gestureRecognizer: UITapGestureRecognizer) {
    delegate?.onFavIconSelected(self)
  }

  private func seekDirectionWithAnimation(_ seekBlock: () -> Void) {
    isSeeking = true
    toggleOverlays(showOverlay: true)
    isOverlayDisplayed = true

    seekBlock()

    fadeAnimationWorkItem?.cancel()
    fadeAnimationWorkItem = DispatchWorkItem(block: { [weak self] in
      guard let self = self else { return }
      self.isSeeking = false
      self.toggleOverlays(showOverlay: false)
      self.isOverlayDisplayed = false
    })

    guard let fadeAnimationWorkItem = fadeAnimationWorkItem else { return }
    DispatchQueue.main.asyncAfter(deadline: .now() + 5.0, execute: fadeAnimationWorkItem)
  }

  @objc
  private func onRepeat(_ button: UIButton) {
    guard let delegate = delegate else { return }
    delegate.toggleRepeatMode(self)
  }

  @objc
  private func onPlay(_ button: UIButton) {
    fadeAnimationWorkItem?.cancel()
    guard let delegate = delegate else { return }

    if delegate.isPlaying {
      self.pause()
    } else {
      self.play()
    }
  }

  @objc
  private func onPlaybackRateChanged(_ button: UIButton) {
    guard let delegate = delegate else { return }

    var playbackRate = delegate.playbackRate
    if playbackRate == 1.0 {
      playbackRate = 1.5
      button.setTitle("1.5x", for: .normal)
    } else if playbackRate == 1.5 {
      playbackRate = 2.0
      button.setTitle("2x", for: .normal)
    } else {
      playbackRate = 1.0
      button.setTitle("1x", for: .normal)
    }

    let wasPlaying = delegate.isPlaying
    delegate.setPlaybackRate(self, rate: playbackRate)

    if wasPlaying {
      delegate.play(self)
      controlsView.playPauseButton.setImage(UIImage(named: "playlist_pause", in: .module, compatibleWith: nil)!, for: .normal)
    } else {
      delegate.pause(self)
      controlsView.playPauseButton.setImage(UIImage(named: "playlist_play", in: .module, compatibleWith: nil)!, for: .normal)
    }
  }

  @objc
  private func onSidePanel(_ button: UIButton) {
    self.delegate?.onSidePanelStateChanged(self)
  }

  @objc
  private func onPictureInPicture(_ button: UIButton) {
    delegate?.onPictureInPicture(self)
  }

  @objc
  private func onFullscreen(_ button: UIButton) {
    isFullscreen = true
    infoView.fullscreenButton.isHidden = true
    infoView.exitButton.isHidden = false
    self.delegate?.onFullscreen(self)
  }

  @objc
  private func onExitFullscreen(_ button: UIButton) {
    if isFullscreen {
      isFullscreen = false
      infoView.fullscreenButton.isHidden = false
      infoView.exitButton.isHidden = true
    }

    self.delegate?.onExitFullscreen(self)
  }

  @objc
  private func onSeekBackwards(_ button: UIButton) {
    seekDirectionWithAnimation {
      self.seekBackwards()
    }
  }

  @objc
  private func onSeekForwards(_ button: UIButton) {
    seekDirectionWithAnimation {
      self.seekForwards()
    }
  }

  @objc
  private func onSeekPrevious(_ gestureRecognizer: UIGestureRecognizer) {
    if gestureRecognizer.state == .began {
      self.delegate?.onPreviousTrack(self, isUserInitiated: true)
    }
  }

  @objc
  private func onSeekNext(_ gestureRecognizer: UIGestureRecognizer) {
    if gestureRecognizer.state == .began {
      self.delegate?.onNextTrack(self, isUserInitiated: true)
    }
  }

  @objc
  private func onNextTrack(_ button: UIButton) {
    self.delegate?.onNextTrack(self, isUserInitiated: true)
  }

  func onValueChanged(_ trackBar: VideoTrackerBar, value: CGFloat) {
    guard let delegate = delegate else { return }

    isSeeking = true

    if delegate.isPlaying {
      wasPlayingBeforeSeeking = true
      playbackRate = delegate.playbackRate
      delegate.pause(self)
    }

    isOverlayDisplayed = true
    delegate.seek(self, relativeOffset: Float(value))
  }

  func onValueEnded(_ trackBar: VideoTrackerBar, value: CGFloat) {
    guard let delegate = delegate else { return }

    isSeeking = false

    if wasPlayingBeforeSeeking {
      delegate.play(self)
      delegate.setPlaybackRate(self, rate: playbackRate)
      wasPlayingBeforeSeeking = false
    }

    if delegate.isPlaying {
      fadeAnimationWorkItem?.cancel()
      toggleOverlays(showOverlay: false, except: [overlayView], display: [overlayView])
      overlayView.alpha = 0.0
      isOverlayDisplayed = false
    } else {
      fadeAnimationWorkItem?.cancel()
      toggleOverlays(showOverlay: true)
      overlayView.alpha = 1.0
      isOverlayDisplayed = true
    }
  }

  func toggleOverlays(showOverlay: Bool) {
    self.toggleOverlays(showOverlay: showOverlay, except: [], display: [])
  }

  private func toggleOverlays(showOverlay: Bool, except: [UIView] = [], display: [UIView] = []) {
    UIView.animate(
      withDuration: 1.0, delay: 0.0, usingSpringWithDamping: 1.0, initialSpringVelocity: 1.0, options: [.curveEaseInOut, .allowUserInteraction],
      animations: {
        self.subviews.forEach({
          if $0 == self.particleView || $0 == self.staticImageView {
            return
          }
          
          if !except.contains($0) {
            $0.alpha = showOverlay ? 1.0 : 0.0
          } else if display.contains($0) {
            $0.alpha = 1.0
          } else {
            $0.alpha = 0.0
          }
        })
      })
  }
  
  func setStaticImage(image: UIImage?) {
    staticImageView.image = image
    staticImageView.isHidden = image == nil
    staticImageView.backgroundColor = .black
    staticImageView.contentMode = .scaleAspectFit
  }

  func setVideoInfo(videoDomain: String, videoTitle: String?, isPrivateBrowsing: Bool) {
    var displayTitle = videoTitle ?? ""

    if displayTitle.isEmpty {
      var hostDomain = ""

      if let host = URL(string: videoDomain)?.baseDomain {
        hostDomain = host
      }

      if hostDomain.hasSuffix("/") {
        hostDomain = String(hostDomain.dropLast())
      }

      if hostDomain.isEmpty {
        hostDomain = videoDomain
      }

      displayTitle = hostDomain
    }

    infoView.titleLabel.text = displayTitle
    infoView.updateFavIcon(domain: videoDomain, isPrivateBrowsing: isPrivateBrowsing)
  }

  func resetVideoInfo() {
    infoView.titleLabel.text = ""
    infoView.clearFavIcon()
    controlsView.trackBar.setTimeRange(currentTime: .zero, endTime: .zero)
  }
  
  func setMediaIsLive(_ isLiveMedia: Bool) {
    self.isMediaLive = isLiveMedia
    controlsView.trackBar.isUserInteractionEnabled = !isLiveMedia
    controlsView.skipBackButton.isEnabled = !isLiveMedia
    controlsView.skipForwardButton.isEnabled = !isLiveMedia
    seekGestures.forEach({ $0.isEnabled = !isLiveMedia })
  }

  func setControlsEnabled(_ enabled: Bool) {
    // Disable all controls except the side-panel and the exit button

    infoView.fullscreenButton.isUserInteractionEnabled = enabled
    infoView.pictureInPictureButton.isUserInteractionEnabled = enabled
    controlsView.isUserInteractionEnabled = enabled

    gestureRecognizers?.forEach({
      $0.isEnabled = enabled && !isMediaLive
    })
  }

  func setFullscreenButtonHidden(_ hidden: Bool) {
    infoView.fullscreenButton.isHidden = hidden
  }

  func setExitButtonHidden(_ hidden: Bool) {
    infoView.exitButton.isHidden = hidden
  }

  func setSidePanelHidden(_ hidden: Bool) {
    infoView.sidePanelButton.isHidden = hidden
  }

  func attachLayer(player: MediaPlayer) {
    playerLayer = player.attachLayer() as? AVPlayerLayer
    if let playerLayer = playerLayer {
      layer.insertSublayer(playerLayer, at: 0)
      layer.insertSublayer(staticImageView.layer, at: 1)
      
      if let player = playerLayer.player {
        playerStatusObserver = player.observe(
          \AVPlayer.currentItem?.tracks, options: [.new],
          changeHandler: { [weak self] _, change in
            guard let self = self else { return }
            
            DispatchQueue.main.async {
              if let tracks = change.newValue,
                 !(tracks?.isEmpty ?? true),
                 !(self.delegate?.isVideoTracksAvailable ?? true) {
                self.particleView.alpha = 1.0
              } else {
                self.particleView.alpha = 0.0
              }
            }
          }
        )
      }
    }
  }

  func play() {
    delegate?.play(self)
  }

  func pause() {
    delegate?.pause(self)
  }

  func stop() {
    delegate?.stop(self)
  }

  func seek(to time: Double) {
    delegate?.seek(self, to: time)
  }

  func seekBackwards() {
    delegate?.seekBackwards(self)
  }

  func seekForwards() {
    delegate?.seekForwards(self)
  }

  func previous() {
    self.delegate?.onPreviousTrack(self, isUserInitiated: false)
  }

  func next() {
    self.delegate?.onNextTrack(self, isUserInitiated: false)
  }
}

extension VideoView: UIGestureRecognizerDelegate {
  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch) -> Bool {
    let location = touch.location(in: self)
    let restrictedViews = [infoView, controlsView]
    return !restrictedViews.contains(where: { $0.point(inside: self.convert(location, to: $0), with: nil) })
  }
}
