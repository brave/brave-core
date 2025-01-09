// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveShared
import Data
import Foundation
import Shared
import UserAgent
import os.log

protocol PlaylistDownloadManagerDelegate: AnyObject {
  func onDownloadProgressUpdate(id: String, percentComplete: Double)
  func onDownloadStateChanged(
    id: String,
    state: PlaylistDownloadManager.DownloadState,
    displayName: String?,
    error: Error?
  )
}

private protocol PlaylistStreamDownloadManagerDelegate: AnyObject {
  // TODO: Should be async, fix when removing legacy playlist UI
  func localAssetSynchronous(for itemId: String) -> AVURLAsset?
  func localAsset(for itemId: String) async -> AVURLAsset?
  func onDownloadProgressUpdate(streamDownloader: Any, id: String, percentComplete: Double)
  func onDownloadStateChanged(
    streamDownloader: Any,
    id: String,
    state: PlaylistDownloadManager.DownloadState,
    displayName: String?,
    error: Error?
  )
}

struct MediaDownloadTask {
  let id: String
  let name: String
  let asset: AVURLAsset
  let pageSrc: String
}

public enum PlaylistDownloadError: Error {
  case uniquePathNotCreated
}

public class PlaylistDownloadManager: PlaylistStreamDownloadManagerDelegate {
  private let hlsSession: AVAssetDownloadURLSession
  private let fileSession: URLSession
  private let dataSession: URLSession
  private let hlsDelegate = PlaylistHLSDownloadManager()
  private let fileDelegate = PlaylistFileDownloadManager()
  private let dataDelegate = PlaylistDataDownloadManager()
  private let hlsQueue = OperationQueue.main
  private let fileQueue = OperationQueue.main
  private let dataQueue = OperationQueue.main

  private var didRestoreSession = false
  weak var delegate: PlaylistDownloadManagerDelegate?

  public static var playlistDirectory: URL? {
    get async {
      try? await AsyncFileManager.default.url(
        for: .applicationSupportDirectory,
        appending: "Playlist",
        create: true
      )
    }
  }

  public enum DownloadState: String {
    case downloaded
    case inProgress
    case invalid
  }

  init() {
    let hlsConfiguration = URLSessionConfiguration.background(
      withIdentifier: "com.brave.playlist.hls.background.session"
    )
    hlsSession = AVAssetDownloadURLSession(
      configuration: hlsConfiguration,
      assetDownloadDelegate: hlsDelegate,
      delegateQueue: hlsQueue
    )

    let fileConfiguration = URLSessionConfiguration.background(
      withIdentifier: "com.brave.playlist.file.background.session"
    )
    fileSession = URLSession(
      configuration: fileConfiguration,
      delegate: fileDelegate,
      delegateQueue: fileQueue
    )

    let dataConfiguration = URLSessionConfiguration.background(
      withIdentifier: "com.brave.playlist.data.background.session"
    )
    dataSession = URLSession(
      configuration: dataConfiguration,
      delegate: dataDelegate,
      delegateQueue: dataQueue
    )

    hlsDelegate.delegate = self
    fileDelegate.delegate = self
    dataDelegate.delegate = self
  }

  func restoreSession(_ completion: @escaping () -> Void) {
    // Called from AppDelegate to restore pending downloads
    guard !didRestoreSession else {
      completion()
      return
    }

    didRestoreSession = true

    let group = DispatchGroup()
    group.enter()
    hlsDelegate.restoreSession(hlsSession) {
      group.leave()
    }

    group.enter()
    fileDelegate.restoreSession(fileSession) {
      group.leave()
    }

    group.enter()
    dataDelegate.restoreSession(dataSession) {
      group.leave()
    }

    group.notify(queue: .main) {
      completion()
    }
  }

  func downloadHLSAsset(_ assetUrl: URL, for item: PlaylistInfo) {
    if Thread.current.isMainThread {
      hlsDelegate.downloadAsset(self.hlsSession, assetUrl: assetUrl, for: item)
    } else {
      hlsQueue.addOperation { [weak self] in
        guard let self = self else { return }
        self.hlsDelegate.downloadAsset(self.hlsSession, assetUrl: assetUrl, for: item)
      }
    }
  }

  func downloadFileAsset(_ assetUrl: URL, for item: PlaylistInfo) {
    if Thread.current.isMainThread {
      fileDelegate.downloadAsset(self.fileSession, assetUrl: assetUrl, for: item)
    } else {
      fileQueue.addOperation { [weak self] in
        guard let self = self else { return }
        self.fileDelegate.downloadAsset(self.fileSession, assetUrl: assetUrl, for: item)
      }
    }
  }

  func downloadDataAsset(_ assetUrl: URL, for item: PlaylistInfo) {
    if Thread.current.isMainThread {
      dataDelegate.downloadAsset(self.fileSession, assetUrl: assetUrl, for: item)
    } else {
      fileQueue.addOperation { [weak self] in
        guard let self = self else { return }
        self.dataDelegate.downloadAsset(self.fileSession, assetUrl: assetUrl, for: item)
      }
    }
  }

  func cancelDownload(itemId: String) {
    if Thread.current.isMainThread {
      hlsDelegate.cancelDownload(itemId: itemId)
      fileDelegate.cancelDownload(itemId: itemId)
      dataDelegate.cancelDownload(itemId: itemId)
    } else {
      hlsQueue.addOperation { [weak self] in
        self?.hlsDelegate.cancelDownload(itemId: itemId)
      }

      fileQueue.addOperation { [weak self] in
        self?.fileDelegate.cancelDownload(itemId: itemId)
      }

      dataQueue.addOperation { [weak self] in
        self?.dataDelegate.cancelDownload(itemId: itemId)
      }
    }
  }

  func downloadTask(for itemId: String) -> MediaDownloadTask? {
    if Thread.current.isMainThread {
      return hlsDelegate.downloadTask(for: itemId) ?? fileDelegate.downloadTask(for: itemId)
        ?? dataDelegate.downloadTask(for: itemId)
    }

    let group = DispatchGroup()

    group.enter()
    var hlsTask: MediaDownloadTask?
    hlsQueue.addOperation { [weak self] in
      defer { group.leave() }
      guard let self = self else { return }
      hlsTask = self.hlsDelegate.downloadTask(for: itemId)
    }

    group.enter()
    var fileTask: MediaDownloadTask?
    fileQueue.addOperation { [weak self] in
      defer { group.leave() }
      guard let self = self else { return }
      fileTask = self.fileDelegate.downloadTask(for: itemId)
    }

    group.enter()
    var dataTask: MediaDownloadTask?
    dataQueue.addOperation { [weak self] in
      defer { group.leave() }
      guard let self = self else { return }
      dataTask = self.dataDelegate.downloadTask(for: itemId)
    }

    group.wait()
    return hlsTask ?? fileTask ?? dataTask
  }

  // MARK: - PlaylistStreamDownloadManagerDelegate

  @available(*, deprecated, renamed: "localAsset(for:)", message: "Use async version")
  func localAssetSynchronous(for itemId: String) -> AVURLAsset? {
    guard let item = PlaylistItem.getItem(uuid: itemId),
      let cachedData = item.cachedData,
      !cachedData.isEmpty
    else { return nil }

    var bookmarkDataIsStale = false
    do {
      let url = try URL(
        resolvingBookmarkData: cachedData,
        bookmarkDataIsStale: &bookmarkDataIsStale
      )

      if bookmarkDataIsStale {
        return nil
      }

      return AVURLAsset(url: url, options: AVAsset.defaultOptions)
    } catch {
      Logger.module.error("\(error.localizedDescription)")
      return nil
    }
  }

  func localAsset(for itemId: String) async -> AVURLAsset? {
    let cachedData = await MainActor.run {
      return PlaylistItem.getItem(uuid: itemId)?.cachedData
    }
    guard let cachedData = cachedData, !cachedData.isEmpty else { return nil }

    var bookmarkDataIsStale = false
    do {
      let url = try URL(
        resolvingBookmarkData: cachedData,
        bookmarkDataIsStale: &bookmarkDataIsStale
      )

      if bookmarkDataIsStale {
        return nil
      }

      return AVURLAsset(url: url, options: AVAsset.defaultOptions)
    } catch {
      Logger.module.error("\(error.localizedDescription)")
      return nil
    }
  }

  fileprivate func onDownloadProgressUpdate(
    streamDownloader: Any,
    id: String,
    percentComplete: Double
  ) {
    delegate?.onDownloadProgressUpdate(id: id, percentComplete: percentComplete)
  }

  fileprivate func onDownloadStateChanged(
    streamDownloader: Any,
    id: String,
    state: PlaylistDownloadManager.DownloadState,
    displayName: String?,
    error: Error?
  ) {
    delegate?.onDownloadStateChanged(id: id, state: state, displayName: displayName, error: error)
  }

  fileprivate static func uniqueDownloadPathForFilename(_ filename: String) async throws -> URL? {
    let filename = Self.stripUnicode(fromFilename: filename)
    let playlistDirectory = await PlaylistDownloadManager.playlistDirectory
    return try playlistDirectory?.uniquePathForFilename(filename)
  }

  // Used to avoid name spoofing using Unicode RTL char to change file extension
  private static func stripUnicode(fromFilename string: String) -> String {
    let validFilenameSet = CharacterSet(charactersIn: ":/")
      .union(.newlines)
      .union(.controlCharacters)
      .union(.illegalCharacters)
    return string.components(separatedBy: validFilenameSet).joined()
  }
}

private class PlaylistHLSDownloadManager: NSObject, AVAssetDownloadDelegate {
  private var activeDownloadTasks = [URLSessionTask: MediaDownloadTask]()
  private var pendingDownloadTasks = [URLSessionTask: URL]()
  private var pendingCancellationTasks = [URLSessionTask]()
  private static let minimumBitRate = 265_000

  weak var delegate: PlaylistStreamDownloadManagerDelegate?

  func restoreSession(_ session: AVAssetDownloadURLSession, completion: @escaping () -> Void) {
    session.getAllTasks { [weak self] tasks in
      defer {
        DispatchQueue.main.async {
          completion()
        }
      }

      guard let self = self else { return }

      for task in tasks {
        // TODO: Investigate progress calculation of AVAggregateAssetDownloadTask
        guard let downloadTask = task as? AVAssetDownloadTask,
          let itemId = task.taskDescription
        else { continue }

        if downloadTask.state != .completed,
          let item = PlaylistItem.getItem(uuid: itemId)
        {
          let info = PlaylistInfo(item: item)
          let asset = MediaDownloadTask(
            id: info.tagId,
            name: info.name,
            asset: downloadTask.urlAsset,
            pageSrc: info.pageSrc
          )
          self.activeDownloadTasks[downloadTask] = asset
        }
      }
    }
  }

  func downloadAsset(_ session: AVAssetDownloadURLSession, assetUrl: URL, for item: PlaylistInfo) {
    let asset = AVURLAsset(url: assetUrl, options: AVAsset.defaultOptions)

    // TODO: In the future switch back to AVAggregateAssetDownloadTask after investigating progress calculation
    //        guard let task =
    //                session.aggregateAssetDownloadTask(with: asset,
    //                                                  mediaSelections: [asset.preferredMediaSelection],
    //                                                  assetTitle: item.name,
    //                                                  assetArtworkData: nil,
    //                                                  options: [AVAssetDownloadTaskMinimumRequiredMediaBitrateKey: PlaylistHLSDownloadManager.minimumBitRate]) else { return }

    guard
      let task =
        session.makeAssetDownloadTask(
          asset: asset,
          assetTitle: item.name,
          assetArtworkData: nil,
          options: [
            AVAssetDownloadTaskMinimumRequiredMediaBitrateKey: PlaylistHLSDownloadManager
              .minimumBitRate
          ]
        )
    else { return }

    task.taskDescription = item.tagId
    activeDownloadTasks[task] = MediaDownloadTask(
      id: item.tagId,
      name: item.name,
      asset: asset,
      pageSrc: item.pageSrc
    )
    task.resume()

    DispatchQueue.main.async {
      self.delegate?.onDownloadStateChanged(
        streamDownloader: self,
        id: item.tagId,
        state: .inProgress,
        displayName: nil,
        error: nil
      )
    }
  }

  func cancelDownload(itemId: String) {
    if let task = activeDownloadTasks.first(where: { $0.value.id == itemId })?.key {
      pendingCancellationTasks.append(task)
      task.cancel()  // will call didCompleteWithError which will cleanup the assets
    }
  }

  func downloadTask(for itemId: String) -> MediaDownloadTask? {
    activeDownloadTasks.first(where: { $0.value.id == itemId })?.value
  }

  // MARK: - AVAssetDownloadTask

  func urlSession(
    _ session: URLSession,
    assetDownloadTask: AVAssetDownloadTask,
    didFinishDownloadingTo location: URL
  ) {
    pendingDownloadTasks[assetDownloadTask] = location
  }

  func urlSession(
    _ session: URLSession,
    assetDownloadTask: AVAssetDownloadTask,
    didLoad timeRange: CMTimeRange,
    totalTimeRangesLoaded loadedTimeRanges: [NSValue],
    timeRangeExpectedToLoad: CMTimeRange
  ) {

    self.urlSession(
      session,
      downloadTask: assetDownloadTask,
      didLoad: timeRange,
      totalTimeRangesLoaded: loadedTimeRanges,
      timeRangeExpectedToLoad: timeRangeExpectedToLoad
    )
  }

  // MARK: - AVAggregateAssetDownloadTask

  func urlSession(
    _ session: URLSession,
    aggregateAssetDownloadTask: AVAggregateAssetDownloadTask,
    willDownloadTo location: URL
  ) {
    pendingDownloadTasks[aggregateAssetDownloadTask] = location
  }

  func urlSession(
    _ session: URLSession,
    aggregateAssetDownloadTask: AVAggregateAssetDownloadTask,
    didLoad timeRange: CMTimeRange,
    totalTimeRangesLoaded loadedTimeRanges: [NSValue],
    timeRangeExpectedToLoad: CMTimeRange,
    for mediaSelection: AVMediaSelection
  ) {
    self.urlSession(
      session,
      downloadTask: aggregateAssetDownloadTask,
      didLoad: timeRange,
      totalTimeRangesLoaded: loadedTimeRanges,
      timeRangeExpectedToLoad: timeRangeExpectedToLoad
    )
  }

  func urlSession(
    _ session: URLSession,
    aggregateAssetDownloadTask: AVAggregateAssetDownloadTask,
    didCompleteFor mediaSelection: AVMediaSelection
  ) {
    guard let asset = activeDownloadTasks[aggregateAssetDownloadTask] else { return }
    aggregateAssetDownloadTask.taskDescription = asset.id
    aggregateAssetDownloadTask.resume()

    DispatchQueue.main.async {
      self.delegate?.onDownloadStateChanged(
        streamDownloader: self,
        id: asset.id,
        state: .inProgress,
        displayName: nil,
        error: nil
      )
    }
  }

  // MARK: - AVAssetDownloadDelegate - Commonly shared code with AVAggregateAssetDownloadTask and AVAssetDownloadTask

  private func urlSession(
    _ session: URLSession,
    downloadTask: URLSessionTask,
    didLoad timeRange: CMTimeRange,
    totalTimeRangesLoaded loadedTimeRanges: [NSValue],
    timeRangeExpectedToLoad: CMTimeRange
  ) {
    guard let asset = activeDownloadTasks[downloadTask] else { return }

    var percentComplete = 0.0
    for value in loadedTimeRanges {
      let loadedTimeRange: CMTimeRange = value.timeRangeValue

      if timeRangeExpectedToLoad.duration.seconds <= 0.0 {
        percentComplete += 0.0
      } else {
        percentComplete +=
          loadedTimeRange.duration.seconds / timeRangeExpectedToLoad.duration.seconds
      }
    }

    DispatchQueue.main.async {
      self.delegate?.onDownloadProgressUpdate(
        streamDownloader: self,
        id: asset.id,
        percentComplete: percentComplete * 100.0
      )
    }
  }

  func urlSession(_ session: URLSession, task: URLSessionTask, didCompleteWithError error: Error?) {
    // Guard against other possible rogue tasks that aren't AVAsset download tasks
    if !(task is AVAssetDownloadTask || task is AVAggregateAssetDownloadTask) {
      return
    }

    let asset = activeDownloadTasks.removeValue(forKey: task)
    let temporaryUrl = pendingDownloadTasks.removeValue(forKey: task)

    guard let asset = asset,
      let temporaryUrl = temporaryUrl
    else { return }

    // This method will delete the downloaded file immediately after this delegate method returns
    // so we must synchonously move the file to a temporary directory first before processing it
    // on a different thread since the Task will execute after this method returns
    let assetUrl = FileManager.default.temporaryDirectory
      .appending(component: "\(asset.id)-\(temporaryUrl.lastPathComponent)")
    try? FileManager.default.moveItem(at: temporaryUrl, to: assetUrl)

    @Sendable func cleanupAndFailDownload(location: URL?, error: Error) async {
      if let location = location {
        do {
          try await AsyncFileManager.default.removeItem(at: location)
        } catch {
          Logger.module.error("Error Deleting Playlist Item: \(error.localizedDescription)")
        }
      }

      await MainActor.run {
        PlaylistItem.updateCache(uuid: asset.id, pageSrc: asset.pageSrc, cachedData: nil)
        self.delegate?.onDownloadStateChanged(
          streamDownloader: self,
          id: asset.id,
          state: .invalid,
          displayName: nil,
          error: error
        )
      }
    }

    Task {
      if let error = error as NSError? {
        switch (error.domain, error.code) {
        case (NSURLErrorDomain, NSURLErrorCancelled):
          // HLS streams can be in two spots, we need to delete from both
          // just in case the download process was in the middle of transferring the asset
          // to its proper location
          if let cacheLocation = await delegate?.localAsset(for: asset.id)?.url {
            do {
              try await AsyncFileManager.default.removeItem(at: cacheLocation)
            } catch {
              Logger.module.error(
                "Could not delete asset cache \(asset.name): \(error.localizedDescription)"
              )
            }
          }

          do {
            try await AsyncFileManager.default.removeItem(atPath: assetUrl.path)
          } catch {
            Logger.module.error(
              "Could not delete asset cache \(asset.name): \(error.localizedDescription)"
            )
          }

          // Update the asset state, but do not propagate the error
          // because the download was cancelled by the user
          if pendingCancellationTasks.contains(task) {
            pendingCancellationTasks.removeAll(where: { $0 == task })

            await MainActor.run {
              PlaylistItem.updateCache(uuid: asset.id, pageSrc: asset.pageSrc, cachedData: nil)
              self.delegate?.onDownloadStateChanged(
                streamDownloader: self,
                id: asset.id,
                state: .invalid,
                displayName: nil,
                error: nil
              )
            }
            return
          }

        case (NSURLErrorDomain, NSURLErrorUnknown):
          Logger.module.error("Downloading HLS streams is not supported on the simulator.")

        default:
          Logger.module.error(
            "An unknown error occurred while attempting to donwload the playlist item: \(error)"
          )
        }

        await MainActor.run {
          Logger.module.debug("\(PlaylistItem.getItem(uuid: asset.id).debugDescription)")
          PlaylistItem.updateCache(uuid: asset.id, pageSrc: asset.pageSrc, cachedData: nil)
          self.delegate?.onDownloadStateChanged(
            streamDownloader: self,
            id: asset.id,
            state: .invalid,
            displayName: nil,
            error: error
          )
        }
      } else {
        do {
          guard
            let path = try await PlaylistDownloadManager.uniqueDownloadPathForFilename(
              assetUrl.lastPathComponent
            )
          else {
            Logger.module.error("Failed to create unique path for playlist item.")
            throw PlaylistDownloadError.uniquePathNotCreated
          }

          try await AsyncFileManager.default.moveItem(at: assetUrl, to: path)
          do {
            let cachedData = try path.bookmarkData()

            await MainActor.run {
              PlaylistItem.updateCache(
                uuid: asset.id,
                pageSrc: asset.pageSrc,
                cachedData: cachedData
              )
              self.delegate?.onDownloadStateChanged(
                streamDownloader: self,
                id: asset.id,
                state: .downloaded,
                displayName: nil,
                error: nil
              )
            }
          } catch {
            Logger.module.error("Failed to create bookmarkData for download URL.")
            await cleanupAndFailDownload(location: path, error: error)
          }
        } catch {
          Logger.module.error(
            "An error occurred attempting to download a playlist item: \(error.localizedDescription)"
          )
          await cleanupAndFailDownload(location: assetUrl, error: error)
        }
      }
    }
  }
}

private class PlaylistFileDownloadManager: NSObject, URLSessionDownloadDelegate {
  private var activeDownloadTasks = [URLSessionTask: MediaDownloadTask]()
  private var pendingCancellationTasks = [URLSessionTask]()

  weak var delegate: PlaylistStreamDownloadManagerDelegate?

  func restoreSession(_ session: URLSession, completion: @escaping () -> Void) {
    session.getAllTasks { [weak self] tasks in
      defer {
        DispatchQueue.main.async {
          completion()
        }
      }

      guard let self = self else { return }

      for task in tasks {
        guard let itemId = task.taskDescription else {
          continue
        }

        DispatchQueue.main.async {
          if task.state != .completed,
            let item = PlaylistItem.getItem(uuid: itemId),
            let assetUrl = URL(string: item.mediaSrc)
          {
            let info = PlaylistInfo(item: item)
            let asset = MediaDownloadTask(
              id: info.tagId,
              name: info.name,
              asset: AVURLAsset(url: assetUrl, options: AVAsset.defaultOptions),
              pageSrc: info.pageSrc
            )
            self.activeDownloadTasks[task] = asset
          }
        }
      }
    }
  }

  func downloadAsset(_ session: URLSession, assetUrl: URL, for item: PlaylistInfo) {
    let asset = AVURLAsset(url: assetUrl, options: AVAsset.defaultOptions)

    let request: URLRequest = {
      var request = URLRequest(
        url: assetUrl,
        cachePolicy: .reloadIgnoringLocalCacheData,
        timeoutInterval: 10.0
      )

      // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Range
      request.addValue("bytes=0-", forHTTPHeaderField: "Range")
      request.addValue(UUID().uuidString, forHTTPHeaderField: "X-Playback-Session-Id")
      request.addValue(UserAgent.userAgentForIdiom(), forHTTPHeaderField: "User-Agent")
      return request
    }()

    let task = session.downloadTask(with: request)

    task.taskDescription = item.tagId
    activeDownloadTasks[task] = MediaDownloadTask(
      id: item.tagId,
      name: item.name,
      asset: asset,
      pageSrc: item.pageSrc
    )
    task.resume()

    DispatchQueue.main.async {
      self.delegate?.onDownloadStateChanged(
        streamDownloader: self,
        id: item.tagId,
        state: .inProgress,
        displayName: nil,
        error: nil
      )
    }
  }

  func cancelDownload(itemId: String) {
    if let task = activeDownloadTasks.first(where: { $0.value.id == itemId })?.key {
      task.cancel()  // will call didCompleteWithError which will cleanup the assets
    }
  }

  func downloadTask(for itemId: String) -> MediaDownloadTask? {
    return activeDownloadTasks.first(where: { $0.value.id == itemId })?.value
  }

  // MARK: - URLSessionDownloadDelegate

  func urlSession(_ session: URLSession, task: URLSessionTask, didCompleteWithError error: Error?) {
    guard let task = task as? URLSessionDownloadTask,
      let asset = activeDownloadTasks.removeValue(forKey: task)
    else { return }

    Task { @MainActor in
      if let error = error as NSError? {
        switch (error.domain, error.code) {
        case (NSURLErrorDomain, NSURLErrorCancelled):
          if let cacheLocation = await delegate?.localAsset(for: asset.id)?.url {
            do {
              try await AsyncFileManager.default.removeItem(at: cacheLocation)
              PlaylistItem.updateCache(uuid: asset.id, pageSrc: asset.pageSrc, cachedData: nil)
            } catch {
              Logger.module.error(
                "Could not delete asset cache \(asset.name): \(error.localizedDescription)"
              )
            }
          }

          // Update the asset state, but do not propagate the error
          // because the download was cancelled by the user
          if pendingCancellationTasks.contains(task) {
            pendingCancellationTasks.removeAll(where: { $0 == task })
            await MainActor.run {
              self.delegate?.onDownloadStateChanged(
                streamDownloader: self,
                id: asset.id,
                state: .invalid,
                displayName: nil,
                error: nil
              )
            }
            return
          }

        case (NSURLErrorDomain, NSURLErrorUnknown):
          assertionFailure("Downloading HLS streams is not supported on the simulator.")

        default:
          assertionFailure(
            "An unknown error occurred while attempting to download the playlist item: \(error.domain)"
          )
        }

        await MainActor.run {
          self.delegate?.onDownloadStateChanged(
            streamDownloader: self,
            id: asset.id,
            state: .invalid,
            displayName: nil,
            error: error
          )
        }
      }
    }
  }

  func urlSession(
    _ session: URLSession,
    downloadTask: URLSessionDownloadTask,
    didWriteData bytesWritten: Int64,
    totalBytesWritten: Int64,
    totalBytesExpectedToWrite: Int64
  ) {
    guard let asset = activeDownloadTasks[downloadTask] else { return }

    if totalBytesExpectedToWrite == NSURLSessionTransferSizeUnknown
      || totalBytesExpectedToWrite == 0
    {
      DispatchQueue.main.async {
        self.delegate?.onDownloadProgressUpdate(
          streamDownloader: self,
          id: asset.id,
          percentComplete: 0.0
        )
      }
    } else {
      let percentage = (Double(totalBytesWritten) / Double(totalBytesExpectedToWrite)) * 100.0

      DispatchQueue.main.async {
        self.delegate?.onDownloadProgressUpdate(
          streamDownloader: self,
          id: asset.id,
          percentComplete: percentage
        )
      }
    }
  }

  private func detectedFileExtension(
    for downloadTask: URLSessionDownloadTask,
    location: URL
  ) async -> String? {
    var detectedFileExtension: String?
    guard let response = downloadTask.response as? HTTPURLResponse else {
      return nil
    }

    // Detect based on File Extension.
    if let url = downloadTask.originalRequest?.url,
      let detectedExtension = PlaylistMimeTypeDetector(url: url).fileExtension
    {
      detectedFileExtension = detectedExtension
    }

    // Detect based on Content-Type header.
    if detectedFileExtension == nil,
      let contentType = response.value(forHTTPHeaderField: "Content-Type"),
      let detectedExtension = PlaylistMimeTypeDetector(mimeType: contentType).fileExtension
    {
      detectedFileExtension = detectedExtension
    }

    // Detect based on Data.
    if detectedFileExtension == nil {
      do {
        let data = try Data(contentsOf: location, options: .mappedIfSafe)
        if let detectedExtension = PlaylistMimeTypeDetector(data: data).fileExtension {
          detectedFileExtension = detectedExtension
        }
      } catch {
        Logger.module.error(
          "Error mapping downloaded playlist file to virtual memory: \(error.localizedDescription)"
        )
      }
    }

    return detectedFileExtension
  }

  func urlSession(
    _ session: URLSession,
    downloadTask: URLSessionDownloadTask,
    didFinishDownloadingTo location: URL
  ) {

    guard let asset = activeDownloadTasks.removeValue(forKey: downloadTask) else { return }

    @Sendable func cleanupAndFailDownload(location: URL?, error: Error) async {
      if let location = location {
        do {
          try await AsyncFileManager.default.removeItem(at: location)
        } catch {
          Logger.module.error("Error Deleting Playlist Item: \(error.localizedDescription)")
        }
      }

      await MainActor.run {
        PlaylistItem.updateCache(uuid: asset.id, pageSrc: asset.pageSrc, cachedData: nil)
        self.delegate?.onDownloadStateChanged(
          streamDownloader: self,
          id: asset.id,
          state: .invalid,
          displayName: nil,
          error: error
        )
      }
    }

    if let response = downloadTask.response as? HTTPURLResponse,
      response.statusCode == 302 || response.statusCode >= 200 && response.statusCode <= 299
    {
      // This method will delete the downloaded file immediately after this delegate method returns
      // so we must synchonously move the file to a temporary directory first before processing it
      // on a different thread since the Task will execute after this method returns
      let temporaryLocation = FileManager.default.temporaryDirectory
        .appending(component: location.lastPathComponent)
      try? FileManager.default.moveItem(at: location, to: temporaryLocation)
      Task {
        // Couldn't determine file type so we assume mp4 which is the most widely used container.
        // If it doesn't work, the video/audio just won't play anyway.
        var fileExtension = "mp4"
        if let detectedFileExtension = await detectedFileExtension(
          for: downloadTask,
          location: temporaryLocation
        ) {
          fileExtension = detectedFileExtension
        }

        do {
          guard
            let path = try await PlaylistDownloadManager.uniqueDownloadPathForFilename(
              asset.name + ".\(fileExtension)"
            )
          else {
            Logger.module.error("Failed to create unique path for playlist item.")
            throw PlaylistDownloadError.uniquePathNotCreated
          }

          try await AsyncFileManager.default.moveItem(at: temporaryLocation, to: path)
          do {
            let cachedData = try path.bookmarkData()

            DispatchQueue.main.async {
              PlaylistItem.updateCache(
                uuid: asset.id,
                pageSrc: asset.pageSrc,
                cachedData: cachedData
              )
              self.delegate?.onDownloadStateChanged(
                streamDownloader: self,
                id: asset.id,
                state: .downloaded,
                displayName: nil,
                error: nil
              )
            }
          } catch {
            Logger.module.error("Failed to create bookmarkData for download URL.")
            await cleanupAndFailDownload(location: path, error: error)
          }
        } catch {
          Logger.module.error(
            "An error occurred attempting to download a playlist item: \(error.localizedDescription)"
          )
          await cleanupAndFailDownload(location: temporaryLocation, error: error)
        }
      }
    } else {
      Task {
        await cleanupAndFailDownload(location: nil, error: URLError(.badServerResponse))
      }
    }
  }
}

private class PlaylistDataDownloadManager: NSObject, URLSessionDataDelegate {
  private var activeDownloadTasks = [URLSessionTask: MediaDownloadTask]()
  private var pendingCancellationTasks = [URLSessionTask]()

  weak var delegate: PlaylistStreamDownloadManagerDelegate?

  func restoreSession(_ session: URLSession, completion: @escaping () -> Void) {
    session.getAllTasks { [weak self] tasks in
      defer {
        DispatchQueue.main.async {
          completion()
        }
      }

      guard let self = self else { return }

      for task in tasks {
        guard let itemId = task.taskDescription else {
          continue
        }

        DispatchQueue.main.async {
          if task.state != .completed,
            let item = PlaylistItem.getItem(uuid: itemId),
            let assetUrl = URL(string: item.mediaSrc)
          {
            let info = PlaylistInfo(item: item)
            let asset = MediaDownloadTask(
              id: info.tagId,
              name: info.name,
              asset: AVURLAsset(url: assetUrl, options: AVAsset.defaultOptions),
              pageSrc: info.pageSrc
            )
            self.activeDownloadTasks[task] = asset
          }
        }
      }
    }
  }

  func downloadAsset(_ session: URLSession, assetUrl: URL, for item: PlaylistInfo) {
    let asset = AVURLAsset(url: assetUrl, options: AVAsset.defaultOptions)

    let request: URLRequest = {
      var request = URLRequest(
        url: assetUrl,
        cachePolicy: .reloadIgnoringLocalCacheData,
        timeoutInterval: 10.0
      )

      // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Range
      request.addValue("bytes=0-", forHTTPHeaderField: "Range")
      request.addValue(UUID().uuidString, forHTTPHeaderField: "X-Playback-Session-Id")
      request.addValue(UserAgent.userAgentForIdiom(), forHTTPHeaderField: "User-Agent")
      return request
    }()

    let task = session.dataTask(with: request)

    task.taskDescription = item.tagId
    activeDownloadTasks[task] = MediaDownloadTask(
      id: item.tagId,
      name: item.name,
      asset: asset,
      pageSrc: item.pageSrc
    )
    task.resume()

    DispatchQueue.main.async {
      self.delegate?.onDownloadStateChanged(
        streamDownloader: self,
        id: item.tagId,
        state: .inProgress,
        displayName: nil,
        error: nil
      )
    }
  }

  func cancelDownload(itemId: String) {
    if let task = activeDownloadTasks.first(where: { $0.value.id == itemId })?.key {
      task.cancel()  // will call didCompleteWithError which will cleanup the assets
    }
  }

  func downloadTask(for itemId: String) -> MediaDownloadTask? {
    return activeDownloadTasks.first(where: { $0.value.id == itemId })?.value
  }

  // MARK: - URLSessionDataDelegate

  func urlSession(_ session: URLSession, task: URLSessionTask, didCompleteWithError error: Error?) {
    guard let task = task as? URLSessionDownloadTask,
      let asset = activeDownloadTasks.removeValue(forKey: task)
    else { return }

    Task {
      if let error = error as NSError? {
        switch (error.domain, error.code) {
        case (NSURLErrorDomain, NSURLErrorCancelled):
          if let cacheLocation = await delegate?.localAsset(for: asset.id)?.url {
            Task {
              do {
                try await AsyncFileManager.default.removeItem(at: cacheLocation)
                PlaylistItem.updateCache(uuid: asset.id, pageSrc: asset.pageSrc, cachedData: nil)
              } catch {
                Logger.module.error(
                  "Could not delete asset cache \(asset.name): \(error.localizedDescription)"
                )
              }
            }
          }

          // Update the asset state, but do not propagate the error
          // because the download was cancelled by the user
          if pendingCancellationTasks.contains(task) {
            pendingCancellationTasks.removeAll(where: { $0 == task })
            await MainActor.run {
              self.delegate?.onDownloadStateChanged(
                streamDownloader: self,
                id: asset.id,
                state: .invalid,
                displayName: nil,
                error: nil
              )
            }
            return
          }

        case (NSURLErrorDomain, NSURLErrorUnknown):
          assertionFailure("Downloading HLS streams is not supported on the simulator.")

        default:
          assertionFailure(
            "An unknown error occurred while attempting to download the playlist item: \(error.domain)"
          )
        }

        await MainActor.run {
          self.delegate?.onDownloadStateChanged(
            streamDownloader: self,
            id: asset.id,
            state: .invalid,
            displayName: nil,
            error: error
          )
        }
      }
    }
  }

  func urlSession(_ session: URLSession, dataTask: URLSessionDataTask, didReceive data: Data) {
    guard let asset = activeDownloadTasks[dataTask] else { return }

    DispatchQueue.main.async {
      self.delegate?.onDownloadProgressUpdate(
        streamDownloader: self,
        id: asset.id,
        percentComplete: 0.0
      )
    }

    @Sendable func cleanupAndFailDownload(location: URL?, error: Error) async {
      if let location = location {
        do {
          try await AsyncFileManager.default.removeItem(at: location)
        } catch {
          Logger.module.error("Error Deleting Playlist Item: \(error.localizedDescription)")
        }
      }

      await MainActor.run {
        PlaylistItem.updateCache(uuid: asset.id, pageSrc: asset.pageSrc, cachedData: nil)
        self.delegate?.onDownloadStateChanged(
          streamDownloader: self,
          id: asset.id,
          state: .invalid,
          displayName: nil,
          error: error
        )
      }
    }
    Task.detached {
      let path = try? await PlaylistDownloadManager.uniqueDownloadPathForFilename(
        asset.name + ".mp4"
      )

      guard let path = path else {
        await MainActor.run {
          self.delegate?.onDownloadStateChanged(
            streamDownloader: self,
            id: asset.id,
            state: .invalid,
            displayName: nil,
            error: PlaylistDownloadError.uniquePathNotCreated
          )
        }
        return
      }

      do {
        try data.write(to: path, options: .atomic)
        do {
          let cachedData = try path.bookmarkData()
          await MainActor.run {
            PlaylistItem.updateCache(uuid: asset.id, pageSrc: asset.pageSrc, cachedData: cachedData)
            self.delegate?.onDownloadStateChanged(
              streamDownloader: self,
              id: asset.id,
              state: .downloaded,
              displayName: nil,
              error: nil
            )
          }
        } catch {
          Logger.module.error("Failed to create bookmarkData for download URL.")
          await cleanupAndFailDownload(location: path, error: error)
        }
      } catch {
        Logger.module.error(
          "An error occurred attempting to download a playlist item: \(error.localizedDescription)"
        )
        await cleanupAndFailDownload(location: path, error: error)
      }
    }

    DispatchQueue.main.async {
      self.delegate?.onDownloadProgressUpdate(
        streamDownloader: self,
        id: asset.id,
        percentComplete: 100.0
      )
    }
  }
}
