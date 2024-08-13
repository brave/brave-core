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
class MediaThumbnailLoader: ObservableObject {
  @Published var image: UIImage?

  enum MediaThumbnailError: Error {
    case invalidURL
    case assetGenerationFailed
  }

  func loadThumbnail(assetURL: URL, pageURL: URL) async throws {
    // The page URL is more stable than the asset URL for most sites, but we don't want to
    // pick up favicons so prefix the cache key.
    let cacheKey = "playlist-\(pageURL.absoluteString)"
    if let cachedImage = SDImageCache.shared.imageFromCache(forKey: cacheKey) {
      await MainActor.run {
        image = cachedImage
      }
      return
    }
    if let oembedThumbnail = await OEmbedThumbnailFetcher.shared.oembedThumbnail(for: pageURL) {
      await MainActor.run {
        image = oembedThumbnail
      }
      await SDImageCache.shared.store(image, forKey: cacheKey)
      return
    }
    if assetURL.scheme == "blob" {
      throw MediaThumbnailError.invalidURL
    }
    // FIXME: Bring over HLSThumbnailGenerator to handle HLS stream thumbnails
    let generator = AVAssetImageGenerator(asset: .init(url: assetURL))
    let cgImage = try await generator.image(at: .init(seconds: 3, preferredTimescale: 1)).image
    try Task.checkCancellation()
    await MainActor.run {
      image = UIImage(cgImage: cgImage)
    }
    await SDImageCache.shared.store(image, forKey: cacheKey)
  }
}

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
      .task(id: pageURL) {
        do {
          try await thumbnailLoader.loadThumbnail(
            assetURL: assetURL,
            pageURL: pageURL
          )
          displayFavicon = false
        } catch {
          displayFavicon = true
        }
      }
  }
}
