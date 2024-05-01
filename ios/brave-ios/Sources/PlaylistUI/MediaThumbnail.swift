// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import CoreMedia
import Favicon
import Foundation
import SDWebImage
import SwiftUI

/// Loads a thumbnail for media assets
@available(iOS 16.0, *)
private class MediaThumbnailLoader: ObservableObject {
  @Published var image: UIImage?

  enum MediaThumbnailError: Error {
    case invalidURL
    case assetGenerationFailed
  }

  @MainActor func loadThumbnail(assetURL: URL, cacheKey: String) async throws {
    if assetURL.scheme == "blob" {
      throw MediaThumbnailError.invalidURL
    }
    if let cachedImage = SDImageCache.shared.imageFromCache(forKey: cacheKey) {
      image = cachedImage
      return
    }
    // FIXME: Figure out if we still need to use something else to deal with HLS
    let generator = AVAssetImageGenerator(asset: .init(url: assetURL))
    image = UIImage(
      cgImage: try await generator.image(at: .init(seconds: 3, preferredTimescale: 1)).image
    )
    await SDImageCache.shared.store(image, forKey: cacheKey)
  }
}

// FIXME: Support oEmbed thumbnails
@available(iOS 16.0, *)
struct MediaThumbnail: View {
  @StateObject private var thumbnailLoader: MediaThumbnailLoader = .init()
  @State private var displayFavicon: Bool = false

  var assetURL: URL
  var pageURL: URL

  var body: some View {
    Color.clear
      .overlay {
        if let image = thumbnailLoader.image {
          Image(uiImage: image)
            .resizable()
            .aspectRatio(contentMode: .fill)
            .transition(.opacity)
        }
      }
      .overlay {
        if displayFavicon {
          FaviconImage(url: pageURL.absoluteString, isPrivateBrowsing: false)
        }
      }
      .task(id: assetURL) {
        do {
          try await thumbnailLoader.loadThumbnail(
            assetURL: assetURL,
            // The page URL is more stable than the asset URL for most sites, but we don't want to
            // pick up favicons so prefix theh cache key.
            cacheKey: "playlist-\(pageURL.absoluteString)"
          )
          displayFavicon = false
        } catch {
          displayFavicon = true
        }
      }
  }
}
