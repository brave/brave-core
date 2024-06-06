// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import MediaPlayer
import Playlist

final class NowPlayingInfo {
  public static func setNowPlayingInfo(_ item: PlaylistInfo, withPlayer player: MediaPlayer) {
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

  public static func updateNowPlayingInfo(_ player: MediaPlayer) {
    let mediaType: MPNowPlayingInfoMediaType =
      player.currentItem?.isVideoTracksAvailable() == true ? .video : .audio
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

  public static func clearNowPlayingInfo() {
    MPNowPlayingInfoCenter.default().nowPlayingInfo = nil
  }

  public static func setNowPlayingMediaArtwork(image: UIImage?) {
    if let image = image {
      let artwork = MPMediaItemArtwork(
        boundsSize: image.size,
        requestHandler: { _ -> UIImage in
          // Do not resize image here.
          // According to Apple it isn't necessary to use expensive resize operations
          return image
        }
      )
      setNowPlayingMediaArtwork(artwork: artwork)
    } else {
      setNowPlayingMediaArtwork(artwork: nil)
    }
  }

  public static func setNowPlayingMediaArtwork(artwork: MPMediaItemArtwork?) {
    if let artwork = artwork {
      var nowPlayingInfo = MPNowPlayingInfoCenter.default().nowPlayingInfo ?? [:]
      nowPlayingInfo[MPMediaItemPropertyArtwork] = artwork
      MPNowPlayingInfoCenter.default().nowPlayingInfo = nowPlayingInfo
    }
  }
}
