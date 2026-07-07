// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AVFoundation
import BraveCore
import BraveShared
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
    // If the item is fully cached we can play it directly without streaming.
    // A cache that is only *in progress* must NOT short-circuit here because the item's stored `src`
    // may be stale or not directly playable. So we still resolve a fresh streamable
    // URL while the cache continues downloading in the background.
    let cacheState = await PlaylistManager.shared.cacheState(for: item.tagId)
    if cacheState == .cached {
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

    // Cache-first: the stored URL is still valid, so `streamingFallback` (which would otherwise kick off caching via `autoDownload`)
    // isn't reached. Start a background cache here so the item is available locally next time. Gated on the kPlaylistCacheFirstEnabled
    // flag so the kPlaylistOfflineCacheEnabled offline-cache path  is unaffected.
    if FeatureList.kPlaylistCacheFirstEnabled.enabled {
      PlaylistManager.shared.autoDownload(item: item)
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

    if Reachability.shared.status.connectionType == .offline {
      Logger.module.error("Couldn't load asset's playability -- Offline")

      // We have no other way of knowing the playable status
      // It is best to assume the item can be played
      // In the worst case, if it can't be played, it will show an error
      return isAssetPlayable()
    }

    // Fetch the playable status asynchronously
    return (try? await asset.load(.isPlayable)) == true
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

      guard var newItem = newItem, URL(string: newItem.src) != nil else {
        throw PlaybackError.cannotLoadMedia
      }

      newItem.pageSrc = item.pageSrc
      newItem.dateAdded = item.dateAdded
      newItem.tagId = item.tagId
      newItem.order = item.order
      newItem.isInvisible = item.isInvisible
      newItem.lastPlayedOffset = 0.0

      let item = try await withCheckedThrowingContinuation { continuation in
        PlaylistItem.updateItem(newItem) { _ in
          continuation.resume(returning: newItem)
        }
      }

      Task {
        try await Task.sleep(nanoseconds: NSEC_PER_SEC)
        PlaylistManager.shared.autoDownload(item: newItem)
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

  // Probes whether `url` can be streamed directly. The mime check is an inexpensive reachability
  // gate; `loadAssetPlayability` is the authoritative check, since a HEAD/mime success can
  // still occur on expired or bad/poisoned URLs (e.g. an HTML consent page) AVPlayer can't play.
  // `defaultOptions` sends the same User-Agent as real playback (some hosts gate on it).
  private func canStreamURL(_ url: URL) async -> Bool {
    guard let mimeType = await PlaylistMediaStreamer.getMimeType(url), !mimeType.isEmpty else {
      return false
    }

    let asset = AVURLAsset(url: url, options: AVAsset.defaultOptions)
    return await PlaylistMediaStreamer.loadAssetPlayability(asset: asset)
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
      request.addValue(UserAgent.mobile, forHTTPHeaderField: "User-Agent")
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
