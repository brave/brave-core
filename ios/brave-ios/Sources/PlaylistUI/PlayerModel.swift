// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AVKit
import BraveStrings
import Combine
import Data
import Foundation
import MediaPlayer
import OrderedCollections
import Playlist
import Preferences
import Strings
import SwiftUI
import os

// FIXME: Add unit tests
public final class PlayerModel: ObservableObject {

  public init(
    mediaStreamer: PlaylistMediaStreamer?,
    initialPlaybackInfo: InitialPlaybackInfo?
  ) {
    self.mediaStreamer = mediaStreamer
    self.initialPlaybackInfo = initialPlaybackInfo

    player.seek(to: .zero)
    player.actionAtItemEnd = .none

    playerLayer.videoGravity = .resizeAspect
    playerLayer.needsDisplayOnBoundsChange = true
    playerLayer.player = player

    pipController = AVPictureInPictureController(playerLayer: playerLayer)
    pipController?.canStartPictureInPictureAutomaticallyFromInline = true
    pipController?.delegate = pictureInPictureDelegate

    setupPlayerKeyPathObservation()
    setupPlayerNotifications()
    setupPictureInPictureKeyPathObservation()
    setupRemoteCommandCenterHandlers()
    Task { @MainActor in
      updateSystemPlayer()
    }

    UIApplication.shared.beginReceivingRemoteControlEvents()

    // FIXME: Maybe only set this before first playback
    DispatchQueue.global().async {
      // should mode be `.moviePlayback`?
      try? AVAudioSession.sharedInstance().setCategory(.playback, mode: .default)
      try? AVAudioSession.sharedInstance().setActive(true)
    }
  }

  deinit {
    if isPlaying {
      log.warning("PlayerModel deallocated without first stopping the underlying media.")
      stop()
    }
    try? AVAudioSession.sharedInstance().setActive(false)
    MPNowPlayingInfoCenter.default().nowPlayingInfo = nil
    UIApplication.shared.endReceivingRemoteControlEvents()
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
    let seconds = item.asset.duration.seconds
    return seconds.isNaN ? 0.0 : seconds
  }

  var currentTimeStream: AsyncStream<TimeInterval> {
    return .init { [weak self] continuation in
      guard let self else { return }
      let observer = player.addCancellablePeriodicTimeObserver(forInterval: 500) { [weak self] _ in
        guard let self else { return }
        continuation.yield(currentTime)
      }
      // Should be tied to the View, but adding one extra killswitch
      self.cancellables.insert(observer)
      continuation.onTermination = { _ in
        observer.cancel()
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

  public func stop() {
    if !isPlaying {
      return
    }
    player.pause()
    currentItem = nil
  }

  private let seekInterval: TimeInterval = 15.0

  func seekBackwards() async {
    guard let currentItem = player.currentItem, currentItem.status == .readyToPlay else {
      return
    }
    await seek(to: currentItem.currentTime().seconds - seekInterval)
  }

  func seekForwards() async {
    guard let currentItem = player.currentItem, currentItem.status == .readyToPlay else {
      return
    }
    await seek(to: currentItem.currentTime().seconds + seekInterval)
  }

  func seek(to time: TimeInterval, accurately: Bool = false) async {
    let seekTime = CMTime(seconds: time, preferredTimescale: 1000)
    await player.seek(
      to: seekTime,
      toleranceBefore: accurately ? .zero : .positiveInfinity,
      toleranceAfter: accurately ? .zero : .positiveInfinity
    )
    await updateSystemPlayer()
  }

  private func handleAudioInterruption(
    type: AVAudioSession.InterruptionType,
    options: AVAudioSession.InterruptionOptions?
  ) {
    switch type {
    case .began:
      pause()
    case .ended:
      if let options, options.contains(.shouldResume) {
        play()
      }
    @unknown default:
      break
    }
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

  @MainActor @Published var repeatMode: RepeatMode = .none {
    didSet {
      updateSystemPlayer()
    }
  }

  @MainActor @Published var isShuffleEnabled: Bool = false {
    didSet {
      if oldValue != isShuffleEnabled {
        makeItemQueue(selectedItemID: selectedItemID)
        updateSystemPlayer()
      }
    }
  }

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

  @MainActor @Published var playbackSpeed: PlaybackSpeed = .normal {
    didSet {
      player.defaultRate = playbackSpeed.rate
      if isPlaying {
        player.rate = playbackSpeed.rate
      }
      updateSystemPlayer()
    }
  }

  private var sleepTimer: Timer?
  @MainActor @Published var sleepTimerFireDate: Date? {
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
      // Should be tied to the View, but adding one extra killswitch
      self.cancellables.insert(timeObserver)
      continuation.onTermination = { _ in
        timeObserver.cancel()
      }
    }
  }

  // MARK: - Picture in Picture

  private var pipController: AVPictureInPictureController?
  public weak var pictureInPictureDelegate: AVPictureInPictureControllerDelegate? {
    didSet {
      pipController?.delegate = pictureInPictureDelegate
    }
  }

  var isPictureInPictureSupported: Bool {
    AVPictureInPictureController.isPictureInPictureSupported()
      && pipController?.isPictureInPicturePossible == true
  }

  public var isPictureInPictureActive: Bool {
    pipController?.isPictureInPictureActive ?? false
  }

  public func startPictureInPicture() {
    pipController?.startPictureInPicture()
  }

  public func stopPictureInPicture() {
    pipController?.stopPictureInPicture()
  }

  private func setupPictureInPictureKeyPathObservation() {
    guard let pipController else { return }
    func subscriber<Value>(
      for keyPath: KeyPath<AVPictureInPictureController, Value>
    ) -> AnyCancellable {
      objectWillChangeSubscriber(on: pipController, for: keyPath)
    }
    cancellables.formUnion([
      subscriber(for: \.isPictureInPicturePossible),
      subscriber(for: \.isPictureInPictureActive),
    ])
  }

  // MARK: - AirPlay

  var allowsExternalPlayback: Bool {
    player.allowsExternalPlayback
  }

  // MARK: - Playlist Queue

  public struct InitialPlaybackInfo {
    public var itemUUID: String
    public var timestamp: TimeInterval

    public init(itemUUID: String, timestamp: TimeInterval) {
      self.itemUUID = itemUUID
      self.timestamp = timestamp
    }
  }

  public let mediaStreamer: PlaylistMediaStreamer?
  public let initialPlaybackInfo: InitialPlaybackInfo?

  @MainActor @Published var selectedItemID: PlaylistItem.ID? {
    willSet {
      if let selectedItem {
        let currentTime = currentTime
        // Reset the current item's last played time if you changed videos in the last 10s
        PlaylistManager.shared.updateLastPlayed(
          item: .init(item: selectedItem),
          playTime: (duration - 10...duration).contains(currentTime) ? 0 : currentTime
        )
      }
    }
    didSet {
      updateSystemPlayer(loadArtwork: true)
    }
  }

  @MainActor @Published var selectedFolderID: PlaylistFolder.ID = PlaylistFolder.savedFolderUUID
  @MainActor @Published var isLoadingStreamingURL: Bool = false

  var itemQueue: OrderedSet<PlaylistItem.ID> = []
  var seekToInitialTimestamp: TimeInterval?

  @MainActor private var selectedFolder: PlaylistFolder? {
    PlaylistFolder.getFolder(uuid: selectedFolderID)
  }

  @MainActor private var selectedItem: PlaylistItem? {
    selectedItemID.flatMap { PlaylistItem.getItem(id: $0) }
  }

  @MainActor public func prepareItemQueue() {
    let firstLoadAutoPlay = Preferences.Playlist.firstLoadAutoPlay.value
    let lastPlayedItemURL = Preferences.Playlist.lastPlayedItemUrl.value
    let resumeFromLastTimePlayed = Preferences.Playlist.playbackLeftOff.value

    defer {
      Task { @MainActor in
        if !isPlaying {
          await self.prepareToPlaySelectedItem(
            initialOffset: seekToInitialTimestamp,
            playImmediately: initialPlaybackInfo != nil || firstLoadAutoPlay
          )
        }
        seekToInitialTimestamp = nil
      }
    }

    if !itemQueue.isEmpty {
      // Already prepared the queue, no need to do so again
      return
    }

    // Make an initial queue without any selected item
    makeItemQueue(selectedItemID: nil)
    // Possibly update the selected folder based on the last item played
    let lastPlayedItem: PlaylistItem? =
      lastPlayedItemURL
      .map { PlaylistItem.getItems(pageSrc: $0) }?.first
    if let initialPlaybackInfo,
      let item = PlaylistItem.getItem(uuid: initialPlaybackInfo.itemUUID)
    {
      // If we're coming from a tab with some initial video data update the folder & item
      seekToInitialTimestamp = initialPlaybackInfo.timestamp
      selectedItemID = item.id
      if let folderID = selectedItem?.playlistFolder?.id {
        selectedFolderID = folderID
      }
      // Remake the item queue with the newly selected item
      makeItemQueue(selectedItemID: selectedItemID)
    } else {
      // Just opening playlist without inherited item data, so restore the previously watched item
      // or just auto-play the first item if the pref is on
      if let lastPlayedItem {
        if let lastPlayedItemParentFolder = lastPlayedItem.playlistFolder {
          selectedFolderID = lastPlayedItemParentFolder.id
        }
        selectedItemID = lastPlayedItem.id
        // Remake the item queue with the newly selected item
        makeItemQueue(selectedItemID: selectedItemID)
      } else {
        // Start the first video in the queue if auto-play is enabled
        if resumeFromLastTimePlayed, let selectedItem {
          seekToInitialTimestamp = selectedItem.lastPlayedOffset
        }
        selectedItemID = itemQueue.first
      }
    }
  }

  @MainActor func makeItemQueue(selectedItemID: PlaylistItem.ID?) {
    let folder =
      selectedItemID.flatMap {
        PlaylistItem.getItem(id: $0)?.playlistFolder
      } ?? selectedFolder
    var queue: OrderedSet<PlaylistItem.ID> = []
    var items = PlaylistItem.getItems(parentFolder: folder).map(\.id)
    if isShuffleEnabled {
      if let selectedItemID {
        items.removeAll(where: { $0 == selectedItemID })
        queue.append(selectedItemID)
      }
      queue.append(contentsOf: items.shuffled())
    } else {
      queue = OrderedSet(items)
    }
    itemQueue = queue
  }

  @MainActor func playPreviousItem() {
    guard let currentItem = selectedItem, currentItem.id != itemQueue.first,
      let currentItemIndex = itemQueue.firstIndex(of: currentItem.id)
    else {
      return
    }

    // This should be safe as we've already checked if the selected item is the first in the queue
    selectedItemID = itemQueue[currentItemIndex - 1]

    Task {
      await self.prepareToPlaySelectedItem(
        initialOffset: seekToInitialTimestamp,
        playImmediately: true
      )
    }
  }

  @MainActor private var nextItemID: PlaylistItem.ID? {
    guard let selectedItemID, let currentItemIndex = itemQueue.firstIndex(of: selectedItemID) else {
      return nil
    }
    if repeatMode == .one {
      // Replay the current video regardless of shuffle state/queue
      return selectedItemID
    }
    if selectedItemID == itemQueue.last {
      if repeatMode == .all {
        // Last item in the set and repeat mode is on, start from the beginning of the queue
        return itemQueue.first
      }
      // Nothing to play if not repeating
      return nil
    }
    // This should be safe as we've already checked if the selected item is the last in the queue
    return itemQueue[currentItemIndex + 1]
  }

  @MainActor func playNextItem() {
    pause()

    let nextItemID = self.nextItemID
    if selectedItemID != nil, nextItemID == selectedItemID {
      // Re-playing the same item again due to repeat mode
      play()
      return
    }

    if nextItemID != nil || player.currentItem == nil {
      // We'll set the selected item to the next item, or nil it out completely if we were never
      // able to set the player item at all due to an error meaning controls wouldn't work anyways
      self.selectedItemID = nextItemID
      Task {
        await self.prepareToPlaySelectedItem(
          initialOffset: seekToInitialTimestamp,
          playImmediately: true
        )
      }
    }
  }

  struct PlayerModelError: LocalizedError {
    enum Reason {
      case loadingStreamingURLFailed(PlaylistMediaStreamer.PlaybackError)
      case unknown
    }
    var reason: Reason
    var handler: (() -> Void)?

    var errorDescription: String? {
      // `errorDescription` is used for the title in SwiftUI's `alert` modifier
      switch reason {
      case .loadingStreamingURLFailed(.expired):
        return Strings.PlayList.expiredAlertTitle
      case .loadingStreamingURLFailed(.cannotLoadMedia):
        return Strings.PlayList.sorryAlertTitle
      default:
        return "Something went wrong"
      }
    }

    var failureReason: String? {
      switch reason {
      case .loadingStreamingURLFailed(.expired):
        return Strings.PlayList.expiredAlertDescription
      case .loadingStreamingURLFailed(.cannotLoadMedia):
        return Strings.PlayList.loadResourcesErrorAlertDescription
      default:
        return nil
      }
    }
  }

  @MainActor var isErrorAlertPresented: Binding<Bool> {
    .init(get: { self.error != nil }, set: { if !$0 { self.error = nil } })
  }
  @MainActor @Published var error: PlayerModelError?

  @MainActor func prepareToPlaySelectedItem(
    initialOffset: TimeInterval?,
    playImmediately: Bool
  ) async {
    guard let item = selectedItem else { return }
    var playerItemToReplace: AVPlayerItem?
    if let cachedDataURL = item.cachedDataURL {
      playerItemToReplace = .init(url: cachedDataURL)
    }
    if playerItemToReplace == nil, let mediaStreamer {
      if !isPictureInPictureActive {
        // Stop the current video and start loading the streaming video, but only if we're not
        // in pip, since setting the item to nil during pip will cause it to close.
        currentItem = nil
      }
      isLoadingStreamingURL = true
      do {
        let newItem = try await mediaStreamer.loadMediaStreamingAsset(.init(item: item))
        if let url = URL(string: newItem.src) {
          playerItemToReplace = .init(asset: AVURLAsset(url: url))
        }
      } catch {
        if isPictureInPictureActive {
          // Can't show any error in PiP, so skip to the next item
          playNextItem()
        } else {
          let reason: PlayerModelError.Reason = {
            if let error = error as? PlaylistMediaStreamer.PlaybackError {
              return .loadingStreamingURLFailed(error)
            }
            return .unknown
          }()
          self.error = .init(
            reason: reason,
            handler: { [weak self] in
              self?.playNextItem()
            }
          )
        }
      }
      isLoadingStreamingURL = false
    }
    let resumeFromLastTimePlayed = Preferences.Playlist.playbackLeftOff.value
    if let playerItem = playerItemToReplace {
      self.currentItem = playerItem
      if let initialOffset {
        await seek(to: initialOffset, accurately: true)
      } else if resumeFromLastTimePlayed {
        await seek(to: item.lastPlayedOffset, accurately: true)
      }
      if playImmediately {
        play()
      }
    }
  }

  // MARK: -

  private let player: AVPlayer = .init()
  private let videoDecorationOutput = AVPlayerItemVideoOutput(pixelBufferAttributes: [
    kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA
  ])
  private var currentItem: AVPlayerItem? {
    get {
      player.currentItem
    }
    set {
      player.replaceCurrentItem(with: newValue)
      setupPlayerItemKeyPathObservation()
      newValue?.add(videoDecorationOutput)
      Task { @MainActor [weak self] in
        self?.updateSystemPlayer()
      }
    }
  }
  private(set) var playerLayer: AVPlayerLayer = .init()

  private var cancellables: Set<AnyCancellable> = []
  private var itemCancellables: Set<AnyCancellable> = []

  /// Sets up NotificationCenter notifications
  private func setupPlayerNotifications() {
    let center = NotificationCenter.default
    let didPlayToEndTime = center.addObserver(
      forName: AVPlayerItem.didPlayToEndTimeNotification,
      object: nil,
      queue: .main
    ) { [weak self] _ in
      guard let self else { return }
      MainActor.assumeIsolated {
        if let selectedItem = self.selectedItem {
          // Reset the play time of the item that just finished
          PlaylistManager.shared.updateLastPlayed(
            item: .init(item: selectedItem),
            playTime: 0
          )
        }
        self.playNextItem()
      }
    }
    let interruption = center.addObserver(
      forName: AVAudioSession.interruptionNotification,
      object: AVAudioSession.sharedInstance(),
      queue: .main,
      using: { [weak self] notification in
        guard let self,
          let userInfo = notification.userInfo,
          let typeInfo = userInfo[AVAudioSessionInterruptionTypeKey] as? UInt,
          let type = AVAudioSession.InterruptionType(rawValue: typeInfo)
        else {
          return
        }
        let options = (userInfo[AVAudioSessionInterruptionOptionKey] as? UInt).map(
          AVAudioSession.InterruptionOptions.init(rawValue:)
        )
        handleAudioInterruption(type: type, options: options)
      }
    )
    cancellables.formUnion([
      .init { _ = didPlayToEndTime },
      .init { _ = interruption },
    ])
  }

  /// Sets up KVO observations for AVPlayer properties which trigger the `objectWillChange` publisher
  private func setupPlayerKeyPathObservation() {
    func subscriber<Value>(for keyPath: KeyPath<AVPlayer, Value>) -> AnyCancellable {
      objectWillChangeSubscriber(on: player, for: keyPath)
    }
    let timeControlStatusObservable = player.observe(\.timeControlStatus, options: [.new]) {
      [weak self] _, _ in
      DispatchQueue.main.async {
        self?.updateSystemPlayer()
      }
    }
    cancellables.formUnion([
      subscriber(for: \.timeControlStatus),
      subscriber(for: \.allowsExternalPlayback),
      .init { timeControlStatusObservable.invalidate() },
    ])
  }

  /// Sets up KVO observations for AVPlayerItem properties which may trigger the `objectWillChange`
  /// publisher
  private func setupPlayerItemKeyPathObservation() {
    itemCancellables.removeAll()
    guard let item = currentItem else { return }
    func subscriber<Value>(for keyPath: KeyPath<AVPlayerItem, Value>) -> AnyCancellable {
      objectWillChangeSubscriber(on: item, for: keyPath)
    }
    let statusObservable = item.observe(\.status) { [weak self] _, _ in
      DispatchQueue.main.async {
        self?.updateSystemPlayer()
      }
    }
    itemCancellables.formUnion([
      subscriber(for: \.presentationSize),
      .init { statusObservable.invalidate() },
    ])
  }

  private func objectWillChangeSubscriber<Object: NSObject, Value>(
    on object: Object,
    for keyPath: KeyPath<Object, Value>
  ) -> AnyCancellable {
    let observation = object.observe(keyPath, options: [.prior]) { [weak self] _, change in
      if change.isPrior {
        self?.objectWillChange.send()
      }
    }
    return .init {
      observation.invalidate()
    }
  }

  private var systemPlayerArtworkFetchTask: Task<Void, Error>?
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

// MARK: -

extension PlaylistItem {
  var cachedDataURL: URL? {
    guard let cachedData else { return nil }
    do {
      var isStale: Bool = false
      let url = try URL(resolvingBookmarkData: cachedData, bookmarkDataIsStale: &isStale)
      if FileManager.default.fileExists(atPath: url.path) {
        return url
      }
    } catch {
      return nil
    }
    return nil
  }

  /// The URL you would attempt to load in an AVAsset or fetch metadata from. This URL may be
  /// the cached file url or the video url found on the webpage
  var assetURL: URL? {
    cachedDataURL ?? URL(string: mediaSrc)
  }
}

// MARK: - System Media Player

extension PlayerModel {
  @MainActor private func updateSystemPlayer(loadArtwork: Bool = false) {
    guard let selectedItem else {
      MPNowPlayingInfoCenter.default().nowPlayingInfo = nil
      return
    }

    let mediaType: MPNowPlayingInfoMediaType =
      currentItem?.isVideoTracksAvailable() == true ? .video : .audio
    let nowPlayingInfo: [String: Any] = [
      MPNowPlayingInfoPropertyAssetURL: selectedItem.pageSrc.asURL as Any,
      MPNowPlayingInfoPropertyElapsedPlaybackTime: currentTime,
      MPNowPlayingInfoPropertyMediaType: mediaType.rawValue,
      MPNowPlayingInfoPropertyPlaybackRate: playbackSpeed.rate,
      MPMediaItemPropertyArtist: selectedItem.pageSrc.asURL?.baseDomain ?? selectedItem.pageSrc,
      MPMediaItemPropertyPlaybackDuration: duration,
      MPMediaItemPropertyTitle: selectedItem.name,
    ]

    MPRemoteCommandCenter.shared().do {
      $0.skipBackwardCommand.preferredIntervals = [.init(value: seekInterval)]
      $0.skipForwardCommand.preferredIntervals = [.init(value: seekInterval)]
      $0.seekBackwardCommand.isEnabled = itemQueue.first != selectedItem.id
      $0.seekForwardCommand.isEnabled = itemQueue.last != selectedItem.id || repeatMode != .none
      $0.changeRepeatModeCommand.currentRepeatType = repeatMode.repeatType
      $0.changeShuffleModeCommand.currentShuffleType = isShuffleEnabled ? .items : .off
      $0.changePlaybackRateCommand.supportedPlaybackRates = PlaybackSpeed.supportedSpeeds.map {
        NSNumber(value: $0.rate)
      }
      $0.ratingCommand.isEnabled = false
      $0.dislikeCommand.isEnabled = false
      $0.likeCommand.isEnabled = false
      $0.bookmarkCommand.isEnabled = false
    }

    MPNowPlayingInfoCenter.default().do {
      $0.nowPlayingInfo = ($0.nowPlayingInfo ?? [:])
        .merging(nowPlayingInfo, uniquingKeysWith: { $1 })
      $0.playbackState = isPlaying ? .playing : currentItem != nil ? .paused : .stopped
    }

    // We only need to load artwork when the selected item changes
    if loadArtwork, let assetURL = selectedItem.assetURL,
      let pageURL = URL(string: selectedItem.pageSrc)
    {
      MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPMediaItemPropertyArtwork] = nil
      systemPlayerArtworkFetchTask?.cancel()
      systemPlayerArtworkFetchTask = Task { [weak self, id = selectedItem.id] in
        let thumbnailLoader = MediaThumbnailLoader()
        try await thumbnailLoader.loadThumbnail(assetURL: assetURL, pageURL: pageURL)
        try Task.checkCancellation()
        guard id == self?.selectedItemID else { return }
        if let image = thumbnailLoader.image {
          let artwork = MPMediaItemArtwork(boundsSize: image.size) { _ in
            image
          }
          MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPMediaItemPropertyArtwork] = artwork
        } else {
          MPNowPlayingInfoCenter.default().nowPlayingInfo?[MPMediaItemPropertyArtwork] = nil
        }
      }
    }
  }

  private func setupRemoteCommandCenterHandlers() {
    let center: MPRemoteCommandCenter = .shared()
    let commands: [MPRemoteCommand] = [
      center.pauseCommand,
      center.playCommand,
      center.stopCommand,
      center.togglePlayPauseCommand,
      center.skipBackwardCommand,
      center.skipForwardCommand,
      center.seekBackwardCommand,
      center.seekForwardCommand,
      center.changeRepeatModeCommand,
      center.changeShuffleModeCommand,
      center.changePlaybackRateCommand,
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
    case center.togglePlayPauseCommand:
      if isPlaying {
        pause()
      } else {
        play()
      }
    case center.skipBackwardCommand:
      Task { await seekBackwards() }
    case center.skipForwardCommand:
      Task { await seekForwards() }
    case center.seekBackwardCommand:
      Task { await playPreviousItem() }
    case center.seekForwardCommand:
      Task { await playNextItem() }
    case center.changeRepeatModeCommand:
      if let repeatType = (event as? MPChangeRepeatModeCommandEvent)?.repeatType,
        let repeatMode = RepeatMode(repeatType: repeatType)
      {
        Task(priority: .userInitiated) { @MainActor in
          self.repeatMode = repeatMode
        }
      }
    case center.changeShuffleModeCommand:
      if let shuffleType = (event as? MPChangeShuffleModeCommandEvent)?.shuffleType {
        Task(priority: .userInitiated) { @MainActor in
          isShuffleEnabled = shuffleType != .off
        }
      }
    case center.changePlaybackRateCommand:
      if let playbackRate = (event as? MPChangePlaybackRateCommandEvent)?.playbackRate,
        let supportedSpeed = PlaybackSpeed.supportedSpeeds.first(where: { $0.rate == playbackRate })
      {
        Task(priority: .userInitiated) { @MainActor in
          playbackSpeed = supportedSpeed
        }
      }
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

extension PlayerModel.RepeatMode {
  init?(repeatType: MPRepeatType) {
    switch repeatType {
    case .off: self = .none
    case .one: self = .one
    case .all: self = .all
    @unknown default: return nil
    }
  }
  var repeatType: MPRepeatType {
    switch self {
    case .none: return .off
    case .one: return .one
    case .all: return .all
    }
  }
}

extension PlayerModel.PlaybackSpeed {
  static var supportedSpeeds: [Self] = [.normal, .fast, .faster]
}

#if DEBUG
extension PlayerModel {
  static let preview: PlayerModel = .init(mediaStreamer: nil, initialPlaybackInfo: nil)
}
#endif
