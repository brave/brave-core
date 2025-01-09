// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import Combine
import Data
import Foundation
import MediaPlayer
import Shared
import Storage
import UserAgent
import WebKit
import os.log

public class PlaylistMediaStreamer {
  private weak var playerView: UIView?
  private weak var certStore: CertStore?
  private var webLoader: (any PlaylistWebLoader)?
  private var webLoaderFactory: any PlaylistWebLoaderFactory

  public enum PlaybackError: Error {
    case none
    case cancelled
    case expired
    case cannotLoadMedia
    case other(Error)
  }

  public init(playerView: UIView, webLoaderFactory: PlaylistWebLoaderFactory) {
    self.playerView = playerView
    self.webLoaderFactory = webLoaderFactory
  }

  @MainActor
  public func loadMediaStreamingAsset(_ item: PlaylistInfo) async throws -> PlaylistInfo {
    // We need to check if the item is cached locally.
    // If the item is cached (downloaded)
    // then we can play it directly without having to stream it.
    let cacheState = await PlaylistManager.shared.downloadState(for: item.tagId)
    if cacheState != .invalid {
      return item
    }

    // Determine if an item can be streamed and stream it directly
    guard !item.src.isEmpty, let url = URL(string: item.src) else {
      // Fallback to the webview because there was no stream URL somehow..
      return try await streamingFallback(item)
    }

    // Try to stream the asset from its url..
    let isStreamable = await canStreamURL(url)

    // Stream failed so fallback to the webview
    // It's possible the URL expired..
    if !isStreamable {
      return try await streamingFallback(item)
    }

    return item
  }

  public static func loadAssetPlayability(asset: AVURLAsset) async -> Bool {
    let isAssetPlayable = { () -> Bool in
      let status = asset.status(of: .isPlayable)
      if case .loaded(let value) = status {
        return value
      }

      return false
    }

    // Performance improvement to check the status first
    // before attempting to load the playable status
    if isAssetPlayable() {
      return true
    }

    switch Reach().connectionStatus() {
    case .offline, .unknown:
      Logger.module.error("Couldn't load asset's playability -- Offline")

      // We have no other way of knowing the playable status
      // It is best to assume the item can be played
      // In the worst case, if it can't be played, it will show an error
      return isAssetPlayable()

    case .online:
      // Fetch the playable status asynchronously
      return (try? await asset.load(.isPlayable)) == true
    }
  }

  // MARK: - Private

  @MainActor
  private func streamingFallback(_ item: PlaylistInfo) async throws -> PlaylistInfo {
    // Fallback to web stream
    try await withTaskCancellationHandler { @MainActor in
      let webLoader = self.webLoaderFactory.makeWebLoader()
      // If we don't do this, youtube shows ads 100% of the time.
      // It's some weird race-condition in WKWebView where the content blockers may not load until
      // The WebView is visible!
      self.playerView?.insertSubview(webLoader, at: 0)
      self.webLoader = webLoader

      guard let url = URL(string: item.pageSrc) else {
        throw PlaybackError.cannotLoadMedia
      }

      let newItem = await webLoader.load(url: url)
      webLoader.removeFromSuperview()
      self.webLoader = nil

      guard let newItem = newItem, URL(string: newItem.src) != nil else {
        throw PlaybackError.cannotLoadMedia
      }

      let updatedItem = PlaylistInfo(
        name: newItem.name,
        src: newItem.src,
        pageSrc: item.pageSrc,  // Keep the same pageSrc
        pageTitle: newItem.pageTitle,
        mimeType: newItem.mimeType,
        duration: newItem.duration,
        lastPlayedOffset: 0.0,
        detected: newItem.detected,
        dateAdded: item.dateAdded,  // Keep the same dateAdded
        tagId: item.tagId,  // Keep the same tagId
        order: item.order,
        isInvisible: item.isInvisible
      )  // Keep the same order

      let item = try await withCheckedThrowingContinuation { continuation in
        PlaylistItem.updateItem(updatedItem) {
          continuation.resume(returning: updatedItem)
        }
      }

      Task {
        try await Task.sleep(nanoseconds: NSEC_PER_SEC)
        PlaylistManager.shared.autoDownload(item: updatedItem)
      }
      return item
    } onCancel: {
      Task { @MainActor in
        webLoader?.stop()
        webLoader?.removeFromSuperview()
        webLoader = nil
      }
    }
  }

  // Would be nice if AVPlayer could detect the mime-type from the URL for my delegate without a head request..
  // This function only exists because I can't figure out why videos from URLs don't play unless I explicitly specify a mime-type..
  private func canStreamURL(_ url: URL) async -> Bool {
    guard let mimeType = await PlaylistMediaStreamer.getMimeType(url), !mimeType.isEmpty else {
      return false
    }

    return true
  }

  // MARK: - Static

  public static func getMimeType(_ url: URL) async -> String? {
    let request: URLRequest = {
      var request = URLRequest(
        url: url,
        cachePolicy: .reloadIgnoringLocalCacheData,
        timeoutInterval: 10.0
      )

      // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Range
      request.addValue("bytes=0-1", forHTTPHeaderField: "Range")
      request.addValue(UUID().uuidString, forHTTPHeaderField: "X-Playback-Session-Id")
      request.addValue(UserAgent.userAgentForIdiom(), forHTTPHeaderField: "User-Agent")
      return request
    }()

    let session = URLSession(configuration: .ephemeral)

    do {
      let (_, response) = try await session.data(for: request)
      session.finishTasksAndInvalidate()

      if let response = response as? HTTPURLResponse,
        response.statusCode == 302 || response.statusCode >= 200 && response.statusCode <= 299
      {
        if let contentType = response.allHeaderFields["Content-Type"] as? String {
          return contentType
        }
        return "video/*"
      }
      return nil
    } catch {
      session.finishTasksAndInvalidate()
      Logger.module.error("Error fetching MimeType: \(error.localizedDescription)")
      return nil
    }
  }
}
