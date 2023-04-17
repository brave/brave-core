// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import AVFoundation
import Combine
import Data
import WebKit
import MediaPlayer
import Shared
import Storage
import os.log

class PlaylistMediaStreamer {
  private weak var playerView: UIView?
  private weak var certStore: CertStore?
  private var webLoader: PlaylistWebLoader?

  enum PlaybackError: Error {
    case none
    case cancelled
    case expired
    case cannotLoadMedia
    case other(Error)
  }

  init(playerView: UIView) {
    self.playerView = playerView
  }

  func loadMediaStreamingAsset(_ item: PlaylistInfo) -> AnyPublisher<PlaylistInfo, PlaybackError> {
    // We need to check if the item is cached locally.
    // If the item is cached (downloaded)
    // then we can play it directly without having to stream it.
    let cacheState = PlaylistManager.shared.state(for: item.tagId)
    if cacheState != .invalid {
      return Future { resolver in
        resolver(.success(item))
      }.eraseToAnyPublisher()
    }

    // Determine if an item can be streamed and stream it directly
    guard !item.src.isEmpty, let url = URL(string: item.src) else {
      // Fallback to the webview because there was no stream URL somehow..
      return self.streamingFallback(item).eraseToAnyPublisher()
    }

    // Try to stream the asset from its url..
    return canStreamURL(url).flatMap { canStream -> AnyPublisher<PlaylistInfo, PlaybackError> in
      // Stream failed so fallback to the webview
      // It's possible the URL expired..
      if !canStream {
        return self.streamingFallback(item).eraseToAnyPublisher()
      }

      return Future { resolver in
        resolver(.success(item))
      }.eraseToAnyPublisher()
    }.eraseToAnyPublisher()
  }
  
  static func loadAssetPlayability(asset: AVURLAsset, completion: @escaping (Bool) -> Void) {
    let isAssetPlayable = { () -> Bool in
      var error: NSError?
      let status = asset.statusOfValue(forKey: "playable", error: &error)
      let isPlayable = status == .loaded

      if let error = error {
        Logger.module.error("Couldn't load asset's playability: \(error.localizedDescription)")
      }
      return isPlayable
    }

    // Performance improvement to check the status first
    // before attempting to load the playable status
    if isAssetPlayable() {
      DispatchQueue.main.async {
        completion(true)
      }
      return
    }

    switch Reach().connectionStatus() {
    case .offline, .unknown:
      Logger.module.error("Couldn't load asset's playability -- Offline")
      DispatchQueue.main.async {
        // We have no other way of knowing the playable status
        // It is best to assume the item can be played
        // In the worst case, if it can't be played, it will show an error
        completion(isAssetPlayable())
      }
    case .online:
      // Fetch the playable status asynchronously
      asset.loadValuesAsynchronously(forKeys: ["playable"]) {
        DispatchQueue.main.async {
          completion(isAssetPlayable())
        }
      }
    }
  }

  // MARK: - Private

  private func streamingFallback(_ item: PlaylistInfo) -> Deferred<AnyPublisher<PlaylistInfo, PlaybackError>> {
    // Fallback to web stream
    return Deferred {
      var cancelled = false

      return Future { [weak self] resolver in
        guard let self = self else {
          resolver(.failure(.cancelled))
          return
        }

        self.webLoader = PlaylistWebLoader().then {
          // If we don't do this, youtube shows ads 100% of the time.
          // It's some weird race-condition in WKWebView where the content blockers may not load until
          // The WebView is visible!
          self.playerView?.insertSubview($0, at: 0)
        }

        if let url = URL(string: item.pageSrc) {
          self.webLoader?.load(url: url) { [weak self] newItem in
            guard let self = self else { return }
            defer {
              // Destroy the web loader when the callback is complete.
              self.webLoader?.removeFromSuperview()
              self.webLoader = nil
            }

            if let newItem = newItem, URL(string: newItem.src) != nil {
              let updatedItem = PlaylistInfo(name: newItem.name,
                                             src: newItem.src,
                                             pageSrc: item.pageSrc,  // Keep the same pageSrc
                                             pageTitle: newItem.pageTitle,
                                             mimeType: newItem.mimeType,
                                             duration: newItem.duration,
                                             lastPlayedOffset: 0.0,
                                             detected: newItem.detected,
                                             dateAdded: item.dateAdded, // Keep the same dateAdded
                                             tagId: item.tagId,  // Keep the same tagId
                                             order: item.order) // Keep the same order
              
              PlaylistItem.updateItem(updatedItem) {
                resolver(.success(updatedItem))
                
                DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
                  PlaylistManager.shared.autoDownload(item: updatedItem)
                }
              }
            } else if cancelled {
              resolver(.failure(.cancelled))
            } else {
              resolver(.failure(.expired))
            }
          }
        } else {
          resolver(.failure(.cannotLoadMedia))
        }
      }.handleEvents(receiveCancel: { [weak self] in
        cancelled = true
        self?.webLoader?.stop()
      }).eraseToAnyPublisher()
    }
  }

  // Would be nice if AVPlayer could detect the mime-type from the URL for my delegate without a head request..
  // This function only exists because I can't figure out why videos from URLs don't play unless I explicitly specify a mime-type..
  private func canStreamURL(_ url: URL) -> Deferred<AnyPublisher<Bool, PlaybackError>> {
    return Deferred {
      return Future { resolver in
        PlaylistMediaStreamer.getMimeType(url) { mimeType in
          if let mimeType = mimeType {
            resolver(.success(!mimeType.isEmpty))
          } else {
            resolver(.success(false))
          }
        }
      }.eraseToAnyPublisher()
    }
  }

  // MARK: - Static

  static func setNowPlayingInfo(_ item: PlaylistInfo, withPlayer player: MediaPlayer) {
    let mediaType: MPNowPlayingInfoMediaType =
      item.mimeType.contains("video") ? .video : .audio

    var nowPlayingInfo = MPNowPlayingInfoCenter.default().nowPlayingInfo ?? [:]
    nowPlayingInfo.merge(with: [
      MPNowPlayingInfoPropertyMediaType: NSNumber(value: mediaType.rawValue),
      MPMediaItemPropertyTitle: item.name,
      MPMediaItemPropertyArtist: URL(string: item.pageSrc)?.baseDomain ?? item.pageSrc,
      MPMediaItemPropertyPlaybackDuration: item.duration,
      MPNowPlayingInfoPropertyPlaybackRate: player.rate,
      MPNowPlayingInfoPropertyAssetURL: URL(string: item.pageSrc) as Any,
      MPNowPlayingInfoPropertyElapsedPlaybackTime: player.currentTime.seconds,
    ])

    MPNowPlayingInfoCenter.default().nowPlayingInfo = nil
    MPNowPlayingInfoCenter.default().nowPlayingInfo = nowPlayingInfo
  }

  static func updateNowPlayingInfo(_ player: MediaPlayer) {
    let mediaType: MPNowPlayingInfoMediaType = player.currentItem?.isVideoTracksAvailable() == true ? .video : .audio
    let duration = player.currentItem?.asset.duration.seconds ?? 0.0

    var nowPlayingInfo = MPNowPlayingInfoCenter.default().nowPlayingInfo ?? [:]
    nowPlayingInfo.merge(with: [
      MPNowPlayingInfoPropertyMediaType: NSNumber(value: mediaType.rawValue),
      MPMediaItemPropertyPlaybackDuration: duration,
      MPNowPlayingInfoPropertyPlaybackRate: player.rate,
      MPNowPlayingInfoPropertyElapsedPlaybackTime: player.currentTime.seconds,
    ])

    MPNowPlayingInfoCenter.default().nowPlayingInfo = nowPlayingInfo
  }

  static func clearNowPlayingInfo() {
    MPNowPlayingInfoCenter.default().nowPlayingInfo = nil
  }

  static func setNowPlayingMediaArtwork(image: UIImage?) {
    if let image = image {
      let artwork = MPMediaItemArtwork(
        boundsSize: image.size,
        requestHandler: { _ -> UIImage in
          // Do not resize image here.
          // According to Apple it isn't necessary to use expensive resize operations
          return image
        })
      setNowPlayingMediaArtwork(artwork: artwork)
    } else {
      setNowPlayingMediaArtwork(artwork: nil)
    }
  }

  static func setNowPlayingMediaArtwork(artwork: MPMediaItemArtwork?) {
    if let artwork = artwork {
      var nowPlayingInfo = MPNowPlayingInfoCenter.default().nowPlayingInfo ?? [:]
      nowPlayingInfo[MPMediaItemPropertyArtwork] = artwork
      MPNowPlayingInfoCenter.default().nowPlayingInfo = nowPlayingInfo
    }
  }

  static func getMimeType(_ url: URL, _ completion: @escaping (String?) -> Void) {
    let request: URLRequest = {
      var request = URLRequest(url: url, cachePolicy: .reloadIgnoringLocalCacheData, timeoutInterval: 10.0)

      // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Range
      request.addValue("bytes=0-1", forHTTPHeaderField: "Range")
      request.addValue(UUID().uuidString, forHTTPHeaderField: "X-Playback-Session-Id")
      request.addValue(UserAgent.shouldUseDesktopMode ? UserAgent.desktop : UserAgent.mobile, forHTTPHeaderField: "User-Agent")
      return request
    }()

    let session = URLSession(configuration: .ephemeral)
    session.dataTask(with: request) { data, response, error in
      DispatchQueue.main.async {
        if let error = error {
          Logger.module.error("Error fetching MimeType: \(error.localizedDescription)")
          return completion(nil)
        }

        if let response = response as? HTTPURLResponse, response.statusCode == 302 || response.statusCode >= 200 && response.statusCode <= 299 {
          if let contentType = response.allHeaderFields["Content-Type"] as? String {
            completion(contentType)
            return
          } else {
            completion("video/*")
            return
          }
        }

        completion(nil)
      }
    }.resume()
    session.finishTasksAndInvalidate()
  }
}
