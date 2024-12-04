// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import Foundation
import UserAgent

extension AVPlayerItem {
  private var isReadyToPlay: Bool {
    if case .loaded = asset.status(of: .tracks) {
      return true
    }
    return false
  }

  /// Returns whether or not the assetTrack has audio tracks OR the asset has audio tracks
  public func isAudioTracksAvailable() -> Bool {
    tracks.filter({ $0.assetTrack?.mediaType == .audio }).isEmpty == false
  }

  public var isVideoTrackAvailable: Bool {
    get async {
      if !isReadyToPlay {
        return true  // Assume video
      }
      do {
        let assetTracks = try await asset.load(.tracks)
        if tracks.isEmpty || assetTracks.isEmpty {
          return true  // Assume video
        }
        if tracks.allSatisfy({ $0.assetTrack == nil }) {
          return true  // Assume video
        }
        // If the only current track types are audio
        if !tracks.allSatisfy({ $0.assetTrack?.mediaType == .audio }) {
          return true  // Assume video
        }

        let assetHasVideoTrack = await asset.isVideoTrackAvailable
        let hasVideoTracks =
          !tracks.filter({ $0.assetTrack?.mediaType == .video }).isEmpty || assetHasVideoTrack

        // Ultra hack
        // Some items `fade` in/out or have an audio track that fades out but no video track
        // In this case, assume video as it is potentially still a video, just with blank frames
        if !hasVideoTracks && currentTime().seconds <= 1.0
          || fabs(duration.seconds - currentTime().seconds) <= 3.0
        {
          return true
        }

        return hasVideoTracks
      } catch {
        return true  // Assume video
      }
    }
  }

  /// Returns whether or not the assetTrack has video tracks OR the asset has video tracks
  /// If called on optional, assume true
  /// We do this because for m3u8 HLS streams,
  /// tracks may not always be available and the particle effect will show even on videos..
  /// It's best to assume this type of media is a video stream.
  @available(iOS, deprecated: 16.0, renamed: "isVideoTrackAvailable")
  public func isVideoTracksAvailable() -> Bool {
    if !isReadyToPlay {
      return true
    }

    if tracks.isEmpty && asset.tracks.isEmpty {
      return true  // Assume video
    }

    // All tracks are null (not loaded yet)
    if tracks.allSatisfy({ $0.assetTrack == nil }) {
      return true  // Assume video
    }

    // If the only current track types are audio
    if !tracks.allSatisfy({ $0.assetTrack?.mediaType == .audio }) {
      return true  // Assume video
    }

    let hasVideoTracks =
      !tracks.filter({ $0.assetTrack?.mediaType == .video }).isEmpty
      || asset.isVideoTracksAvailable()

    // Ultra hack
    // Some items `fade` in/out or have an audio track that fades out but no video track
    // In this case, assume video as it is potentially still a video, just with blank frames
    if !hasVideoTracks && currentTime().seconds <= 1.0
      || fabs(duration.seconds - currentTime().seconds) <= 3.0
    {
      return true
    }

    return hasVideoTracks
  }
}

extension AVAsset {
  /// Returns whether or not the asset has audio tracks
  @available(iOS, deprecated: 16.0, renamed: "isAudioTrackAvailable")
  public func isAudioTracksAvailable() -> Bool {
    !tracks.filter({ $0.mediaType == .audio }).isEmpty
  }

  /// Returns whether or not the asset has at least one audio track
  public var isAudioTrackAvailable: Bool {
    get async {
      do {
        return try await load(.tracks)
          .contains(where: { $0.mediaType == .audio })
      } catch {
        return false
      }
    }
  }

  /// Returns whether or not the  asset has video tracks
  /// If called on optional, assume true
  /// We do this because for m3u8 HLS streams,
  /// tracks may not always be available and the particle effect will show even on videos..
  /// It's best to assume this type of media is a video stream.
  @available(iOS, deprecated: 16.0, renamed: "isVideoTrackAvailable")
  public func isVideoTracksAvailable() -> Bool {
    !tracks.filter({ $0.mediaType == .video }).isEmpty
  }

  public var isVideoTrackAvailable: Bool {
    get async {
      do {
        return try await load(.tracks)
          .contains(where: { $0.mediaType == .video })
      } catch {
        return false
      }
    }
  }

  public static var defaultOptions: [String: Any] {
    let userAgent = UserAgent.userAgentForIdiom()
    var options: [String: Any] = [:]
    options[AVURLAssetHTTPUserAgentKey] = userAgent
    return options
  }
}
