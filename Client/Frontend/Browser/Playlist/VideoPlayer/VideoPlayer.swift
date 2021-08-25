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

private let log = Logger.browserLogger

public enum VideoViewRepeatMode {
    case none
    case repeatOne
    case repeatAll
}

public protocol VideoViewDelegate: AnyObject {
    func onPreviousTrack(isUserInitiated: Bool)
    func onNextTrack(isUserInitiated: Bool)
    func onSidePanelStateChanged()
    func onPictureInPicture(enabled: Bool)
    func onFullScreen()
    func onExitFullScreen()
}

public class VideoView: UIView, VideoTrackerBarDelegate {
    
    weak var delegate: VideoViewDelegate?
    
    public let player = AVPlayer(playerItem: nil).then {
        $0.seek(to: .zero)
        $0.actionAtItemEnd = .none
    }
    
    public let playerLayer = AVPlayerLayer().then {
        $0.videoGravity = .resizeAspect
        $0.needsDisplayOnBoundsChange = true
    }
    
    private(set) public var pendingMediaItem: AVPlayerItem?
    
    private var isLiveMedia: Bool {
        return (player.currentItem ?? pendingMediaItem)?.asset.duration.isIndefinite == true
    }
    
    private var requestedPlaybackRate = 1.0
    
    private let particleView = PlaylistParticleEmitter().then {
        $0.isHidden = false
        $0.contentMode = .scaleAspectFit
        $0.clipsToBounds = true
    }
    
    private let overlayView = UIImageView().then {
        $0.contentMode = .scaleAspectFit
        $0.isUserInteractionEnabled = true
        $0.backgroundColor = #colorLiteral(red: 0, green: 0, blue: 0, alpha: 0.4024561216)
    }
    
    private let infoView = VideoPlayerInfoBar().then {
        $0.layer.cornerRadius = 18.0
        $0.layer.masksToBounds = true
    }
    
    private let controlsView = VideoPlayerControlsView().then {
        $0.layer.cornerRadius = 18.0
        $0.layer.masksToBounds = true
    }
    
    // State
    private let orientation: UIInterfaceOrientation = .portrait
    private var playObserver: Any?
    private var fadeAnimationWorkItem: DispatchWorkItem?
    
    public var isPlaying: Bool {
        // It is better NOT to keep tracking of isPlaying OR rate > 0.0
        // Instead we should use the timeControlStatus because PIP and Background play
        // via control-center will modify the timeControlStatus property
        // This will keep our UI consistent with what is on the lock-screen.
        // This will also allow us to properly determine play state in
        // PlaylistMediaInfo -> init -> MPRemoteCommandCenter.shared().playCommand
        return player.timeControlStatus == .playing
    }
    private var wasPlayingBeforeSeeking = false
    private(set) public var isSeeking = false
    private(set) public var isFullscreen = false
    private(set) public var isOverlayDisplayed = false
    private(set) public var repeatState: VideoViewRepeatMode = .none
    private var notificationObservers = [NSObjectProtocol]()
    private var pictureInPictureObservers = [NSObjectProtocol]()
    private(set) public var pictureInPictureController: AVPictureInPictureController?
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        
        do {
            try AVAudioSession.sharedInstance().setCategory(.playback, mode: .default)
            try AVAudioSession.sharedInstance().setActive(true, options: [])
        } catch {
            log.error(error)
        }

        // Setup
        backgroundColor = .black
        playerLayer.player = self.player

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
        layer.addSublayer(playerLayer)
        addSubview(particleView)
        addSubview(overlayView)
        addSubview(infoView)
        addSubview(controlsView)
        
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

        registerNotifications()
        registerPictureInPictureNotifications()
        controlsView.trackBar.delegate = self
        
        let overlayTappedGesture = UITapGestureRecognizer(target: self, action: #selector(onOverlayTapped(_:))).then {
            $0.numberOfTapsRequired = 1
            $0.numberOfTouchesRequired = 1
        }
        
        let overlayDoubleTappedGesture = UITapGestureRecognizer(target: self, action: #selector(onOverlayDoubleTapped(_:))).then {
            $0.numberOfTapsRequired = 2
            $0.numberOfTouchesRequired = 1
            $0.delegate = self
        }
        
        addGestureRecognizer(overlayTappedGesture)
        addGestureRecognizer(overlayDoubleTappedGesture)
        overlayTappedGesture.require(toFail: overlayDoubleTappedGesture)
        
        self.toggleOverlays(showOverlay: true)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    deinit {
        do {
            try AVAudioSession.sharedInstance().setCategory(.playback, mode: .default, policy: .longFormAudio, options: [.allowAirPlay, .allowBluetooth, .duckOthers])
            try AVAudioSession.sharedInstance().setActive(false, options: .notifyOthersOnDeactivation)
        } catch {
            log.error(error)
        }
        
        if let observer = self.playObserver {
            player.removeTimeObserver(observer)
        }
        
        notificationObservers.forEach({
            NotificationCenter.default.removeObserver($0)
        })
        
        pictureInPictureObservers.removeAll()
    }
    
    public override func layoutSubviews() {
        super.layoutSubviews()
        
        playerLayer.frame = self.bounds
    }
    
    @objc
    private func onOverlayTapped(_ gestureRecognizer: UITapGestureRecognizer) {
        if isSeeking {
            toggleOverlays(showOverlay: true, except: [overlayView, infoView, controlsView.playPauseButton], display: [controlsView.trackBar])
        } else if (isPlaying && !isOverlayDisplayed) || (!isPlaying && !isSeeking && !isOverlayDisplayed) {
            toggleOverlays(showOverlay: true)
            isOverlayDisplayed = true
            
            fadeAnimationWorkItem?.cancel()
            fadeAnimationWorkItem = DispatchWorkItem(block: { [weak self] in
                guard let self = self else { return }
                self.isOverlayDisplayed = false
                if self.isPlaying && !self.isSeeking {
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
        switch repeatState {
        case .none:
            repeatState = .repeatOne
            controlsView.repeatButton.setImage(#imageLiteral(resourceName: "playlist_repeat_one"), for: .normal)
        case .repeatOne:
            repeatState = .repeatAll
            controlsView.repeatButton.setImage(#imageLiteral(resourceName: "playlist_repeat_all"), for: .normal)
        case .repeatAll:
            repeatState = .none
            controlsView.repeatButton.setImage(#imageLiteral(resourceName: "playlist_repeat"), for: .normal)
        }
    }
    
    @objc
    private func onPlay(_ button: UIButton) {
        fadeAnimationWorkItem?.cancel()
        
        if self.isPlaying {
            self.pause()
        } else {
            self.play()
        }
    }
    
    @objc
    private func onPlaybackRateChanged(_ button: UIButton) {
        if requestedPlaybackRate == 1.0 {
            requestedPlaybackRate = 1.5
            button.setTitle("1.5x", for: .normal)
        } else if requestedPlaybackRate == 1.5 {
            requestedPlaybackRate = 2.0
            button.setTitle("2x", for: .normal)
        } else {
            requestedPlaybackRate = 1.0
            button.setTitle("1x", for: .normal)
        }
        
        player.rate = Float(requestedPlaybackRate)
        controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_pause"), for: .normal)
    }
    
    @objc
    private func onSidePanel(_ button: UIButton) {
        self.delegate?.onSidePanelStateChanged()
    }
    
    @objc
    private func onPictureInPicture(_ button: UIButton) {
        guard let pictureInPictureController = pictureInPictureController else { return }
        
        DispatchQueue.main.async {
            if pictureInPictureController.isPictureInPictureActive {
                self.delegate?.onPictureInPicture(enabled: false)
                pictureInPictureController.stopPictureInPicture()
            } else {
                if #available(iOS 14.0, *) {
                    pictureInPictureController.requiresLinearPlayback = false
                }
                
                self.delegate?.onPictureInPicture(enabled: true)
                pictureInPictureController.startPictureInPicture()
            }
        }
    }
    
    @objc
    private func onFullscreen(_ button: UIButton) {
        isFullscreen = true
        infoView.fullscreenButton.isHidden = true
        infoView.exitButton.isHidden = false
        self.delegate?.onFullScreen()
    }
    
    @objc
    private func onExitFullscreen(_ button: UIButton) {
        isFullscreen = false
        infoView.fullscreenButton.isHidden = false
        infoView.exitButton.isHidden = true
        self.delegate?.onExitFullScreen()
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
            self.delegate?.onPreviousTrack(isUserInitiated: true)
        }
    }
    
    @objc
    private func onSeekNext(_ gestureRecognizer: UIGestureRecognizer) {
        if gestureRecognizer.state == .began {
            self.delegate?.onNextTrack(isUserInitiated: true)
        }
    }
    
    @objc
    private func onNextTrack(_ button: UIButton) {
        self.delegate?.onNextTrack(isUserInitiated: true)
    }
    
    func onValueChanged(_ trackBar: VideoTrackerBar, value: CGFloat) {
        isSeeking = true
        
        if isPlaying {
            player.pause()
            wasPlayingBeforeSeeking = true
        }
        
        toggleOverlays(showOverlay: false, except: [infoView, controlsView], display: [controlsView])
        isOverlayDisplayed = true
        
        if let currentItem = player.currentItem {
            let seekTime = CMTimeMakeWithSeconds(Float64(value * CGFloat(currentItem.asset.duration.value) / CGFloat(currentItem.asset.duration.timescale)), preferredTimescale: currentItem.currentTime().timescale)
            player.seek(to: seekTime)
        }
    }
    
    func onValueEnded(_ trackBar: VideoTrackerBar, value: CGFloat) {
        isSeeking = false
        
        if wasPlayingBeforeSeeking {
            player.play()
            player.rate = Float(requestedPlaybackRate)
            wasPlayingBeforeSeeking = false
        }
        
        if isPlaying || player.rate > 0.0 {
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
    
    private func registerNotifications() {
        notificationObservers.append(NotificationCenter.default.addObserver(forName: UIApplication.didEnterBackgroundNotification, object: nil, queue: .main) { [weak self] _ in
            guard let self = self else { return }
            
            if self.pictureInPictureController?.isPictureInPictureActive == false {
                self.playerLayer.player = nil
            }
        })
        
        notificationObservers.append(NotificationCenter.default.addObserver(forName: UIApplication.didBecomeActiveNotification, object: nil, queue: .main) { [weak self] _ in
            guard let self = self else { return }
            
            if self.pictureInPictureController?.isPictureInPictureActive == false {
                self.playerLayer.player = self.player
            }
        })
        
        notificationObservers.append(NotificationCenter.default.addObserver(forName: .AVPlayerItemDidPlayToEndTime, object: self.player.currentItem, queue: .main) { [weak self] _ in
            guard let self = self, let currentItem = self.player.currentItem else { return }
            
            self.controlsView.playPauseButton.isEnabled = false
            self.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_pause"), for: .normal)
            self.player.pause()
            
            let endTime = CMTimeConvertScale(currentItem.asset.duration, timescale: self.player.currentTime().timescale, method: .roundHalfAwayFromZero)
            
            self.controlsView.trackBar.setTimeRange(currentTime: currentItem.currentTime(), endTime: endTime)
            self.player.seek(to: .zero)
            
            self.controlsView.playPauseButton.isEnabled = true
            self.controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)

            self.toggleOverlays(showOverlay: true)
            
            self.next()
        })
        
        let interval = CMTimeMake(value: 25, timescale: 1000)
        self.playObserver = self.player.addPeriodicTimeObserver(forInterval: interval, queue: .main, using: { [weak self] time in
            guard let self = self, let currentItem = self.player.currentItem else { return }
            
            let endTime = CMTimeConvertScale(currentItem.asset.duration, timescale: self.player.currentTime().timescale, method: .roundHalfAwayFromZero)
            
            if CMTimeCompare(endTime, .zero) != 0 && endTime.value > 0 {
                self.controlsView.trackBar.setTimeRange(currentTime: self.player.currentTime(), endTime: endTime)
            }
        })
    }
    
    private func registerPictureInPictureNotifications() {
        if AVPictureInPictureController.isPictureInPictureSupported() {
            pictureInPictureController = AVPictureInPictureController(playerLayer: self.playerLayer)
            guard let pictureInPictureController = pictureInPictureController else { return }
            
            pictureInPictureObservers.append(pictureInPictureController.observe(\AVPictureInPictureController.isPictureInPicturePossible, options: [.initial, .new]) { [weak self] _, change in
                self?.infoView.pictureInPictureButton.isEnabled = change.newValue ?? false
            })
        } else {
            infoView.pictureInPictureButton.isEnabled = false
        }
    }
    
    private func toggleOverlays(showOverlay: Bool) {
        self.toggleOverlays(showOverlay: showOverlay, except: [], display: [])
    }
    
    private func toggleOverlays(showOverlay: Bool, except: [UIView] = [], display: [UIView] = []) {
        var except = except
        var display = display
        
        if isVideoAvailable() {
            if showOverlay {
                except.append(particleView)
            }
        } else {
            // If the overlay is showing, hide the particle view.. else show it..
            except.append(particleView)
            display.append(particleView)
        }
        
        UIView.animate(withDuration: 1.0, delay: 0.0, usingSpringWithDamping: 1.0, initialSpringVelocity: 1.0, options: [.curveEaseInOut, .allowUserInteraction], animations: {
            self.subviews.forEach({
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
    
    public func setVideoInfo(videoDomain: String, videoTitle: String?) {
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
        infoView.updateFavIcon(domain: videoDomain)
    }
    
    public func resetVideoInfo() {
        infoView.titleLabel.text = ""
        infoView.clearFavIcon()
    }
    
    public func setControlsEnabled(_ enabled: Bool) {
        // Disable all controls except the side-panel and the exit button
        
        infoView.fullscreenButton.isUserInteractionEnabled = enabled
        infoView.pictureInPictureButton.isUserInteractionEnabled = enabled
        controlsView.isUserInteractionEnabled = enabled
        
        gestureRecognizers?.forEach({
            $0.isEnabled = enabled
        })
    }
    
    public func setFullscreenButtonHidden(_ hidden: Bool) {
        infoView.fullscreenButton.isHidden = hidden
    }
    
    public func setExitButtonHidden(_ hidden: Bool) {
        infoView.exitButton.isHidden = hidden
    }
    
    public func setSidePanelHidden(_ hidden: Bool) {
        infoView.sidePanelButton.isHidden = hidden
    }
    
    public func attachLayer() {
        layer.insertSublayer(playerLayer, at: 0)
        playerLayer.player = player
    }
    
    public func detachLayer() {
        playerLayer.removeFromSuperlayer()
        playerLayer.player = nil
    }
    
    public func play() {
        if isPlaying {
            toggleOverlays(showOverlay: isOverlayDisplayed)
        } else {
            controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_pause"), for: .normal)
            player.play()
            
            toggleOverlays(showOverlay: false)
            isOverlayDisplayed = false
        }
    }
    
    public func pause() {
        if isPlaying {
            controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)
            player.pause()
            
            toggleOverlays(showOverlay: true)
            isOverlayDisplayed = true
        } else {
            toggleOverlays(showOverlay: isOverlayDisplayed)
        }
    }
    
    public func stop() {
        controlsView.playPauseButton.setImage(#imageLiteral(resourceName: "playlist_play"), for: .normal)
        player.pause()
        
        toggleOverlays(showOverlay: true)
        isOverlayDisplayed = true
        player.replaceCurrentItem(with: nil)
    }
    
    public func seek(to time: Double) {
        if let currentItem = player.currentItem {
            var seekTime = time
            if seekTime < 0.0 {
                seekTime = 0.0
            }
            
            if seekTime >= currentItem.duration.seconds {
                seekTime = currentItem.duration.seconds
            }
            
            let absoluteTime = CMTimeMakeWithSeconds(seekTime, preferredTimescale: currentItem.currentTime().timescale)
            player.seek(to: absoluteTime, toleranceBefore: .zero, toleranceAfter: .zero)
        }
    }
    
    public func seekBackwards() {
        if let currentItem = player.currentItem {
            let currentTime = currentItem.currentTime().seconds
            var seekTime = currentTime - 15.0

            if seekTime < 0 {
                seekTime = 0
            }
            
            let absoluteTime = CMTimeMakeWithSeconds(seekTime, preferredTimescale: currentItem.currentTime().timescale)
            player.seek(to: absoluteTime, toleranceBefore: .zero, toleranceAfter: .zero)
        }
    }
    
    public func seekForwards() {
        if let currentItem = player.currentItem {
            let currentTime = currentItem.currentTime().seconds
            let seekTime = currentTime + 15.0

            if seekTime < (currentItem.duration.seconds - 15.0) {
                let absoluteTime = CMTimeMakeWithSeconds(seekTime, preferredTimescale: currentItem.currentTime().timescale)
                player.seek(to: absoluteTime, toleranceBefore: .zero, toleranceAfter: .zero)
            }
        }
    }
    
    public func previous() {
        self.delegate?.onPreviousTrack(isUserInitiated: false)
    }
    
    public func next() {
        self.delegate?.onNextTrack(isUserInitiated: false)
    }
    
    public func load(url: URL, resourceDelegate: AVAssetResourceLoaderDelegate?, autoPlayEnabled: Bool, completion: (() -> Void)?) {
        let asset = AVURLAsset(url: url)
        
        if let delegate = resourceDelegate {
            asset.resourceLoader.setDelegate(delegate, queue: .main)
        }
        
        if let currentItem = player.currentItem, currentItem.asset.isKind(of: AVURLAsset.self) && player.status == .readyToPlay {
            if let asset = currentItem.asset as? AVURLAsset, asset.url.absoluteString == url.absoluteString {
                if isPlaying {
                    self.pause()
                    self.play()
                }
                
                self.pendingMediaItem = nil
                DispatchQueue.main.async {
                    completion?()
                }
                return
            }
        }
        
        self.pendingMediaItem = AVPlayerItem(asset: asset)
        asset.loadValuesAsynchronously(forKeys: ["playable", "tracks", "duration"]) { [weak self] in
            guard let self = self, let item = self.pendingMediaItem else { return }
            DispatchQueue.main.async {
                self.player.replaceCurrentItem(with: item)
                self.pendingMediaItem = nil
                
                // Live media item
                let isPlayingLiveMedia = self.isLiveMedia
                self.controlsView.trackBar.isUserInteractionEnabled = !isPlayingLiveMedia
                self.controlsView.skipBackButton.isEnabled = !isPlayingLiveMedia
                self.controlsView.skipForwardButton.isEnabled = !isPlayingLiveMedia
                
                let endTime = CMTimeConvertScale(item.asset.duration, timescale: self.player.currentTime().timescale, method: .roundHalfAwayFromZero)
                self.controlsView.trackBar.setTimeRange(currentTime: item.currentTime(), endTime: endTime)
                
                if autoPlayEnabled {
                    self.play()
                }
                
                DispatchQueue.main.async {
                    completion?()
                }
            }
        }
    }
    
    public func load(asset: AVURLAsset, autoPlayEnabled: Bool, completion: (() -> Void)?) {
        if let currentItem = player.currentItem, currentItem.asset.isKind(of: AVURLAsset.self) && player.status == .readyToPlay {
            if let currentAsset = currentItem.asset as? AVURLAsset, currentAsset.url.absoluteString == asset.url.absoluteString {
                if isPlaying {
                    self.pause()
                    self.play()
                }
                
                self.pendingMediaItem = nil
                DispatchQueue.main.async {
                    completion?()
                }
                return
            }
        }
        
        self.pendingMediaItem = AVPlayerItem(asset: asset)
        asset.loadValuesAsynchronously(forKeys: ["playable", "tracks", "duration"]) { [weak self] in
            guard let self = self, let item = self.pendingMediaItem else { return }
            DispatchQueue.main.async {
                self.player.replaceCurrentItem(with: item)
                self.pendingMediaItem = nil
                
                // Live media item
                let isPlayingLiveMedia = self.isLiveMedia
                self.controlsView.trackBar.isUserInteractionEnabled = !isPlayingLiveMedia
                self.controlsView.skipBackButton.isEnabled = !isPlayingLiveMedia
                self.controlsView.skipForwardButton.isEnabled = !isPlayingLiveMedia
                
                let endTime = CMTimeConvertScale(item.asset.duration, timescale: self.player.currentTime().timescale, method: .roundHalfAwayFromZero)
                self.controlsView.trackBar.setTimeRange(currentTime: item.currentTime(), endTime: endTime)
                
                if autoPlayEnabled {
                    self.play()
                }
                
                DispatchQueue.main.async {
                    completion?()
                }
            }
        }
    }
    
    public func checkInsideTrackBar(point: CGPoint) -> Bool {
        controlsView.trackBar.frame.contains(point)
    }
    
    private func isAudioAvailable() -> Bool {
        if let tracks = self.player.currentItem?.asset.tracks {
            return tracks.filter({ $0.mediaType == .audio }).isEmpty == false
        }
        return false
    }

    private func isVideoAvailable() -> Bool {
        if let tracks = self.player.currentItem?.asset.tracks {
            return tracks.isEmpty || tracks.filter({ $0.mediaType == .video }).isEmpty == false
        }
        
        // We do this because for m3u8 HLS streams,
        // tracks may not always be available and the particle effect will show even on videos..
        // It's best to assume this type of media is a video stream.
        return true
    }
}

extension VideoView: UIGestureRecognizerDelegate {
    public func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch) -> Bool {
        let location = touch.location(in: self)
        let restrictedViews = [infoView, controlsView]
        for view in restrictedViews {
            if view.point(inside: self.convert(location, to: view), with: nil) {
                return false
            }
        }
        return true
    }
}
