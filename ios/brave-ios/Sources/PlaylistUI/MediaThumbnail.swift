// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import AVKit
import CoreMedia
import Foundation
import SDWebImage
import SwiftUI

/// Loads a thumbnail for media assets
@available(iOS 16.0, *)
private class MediaThumbnailLoader: ObservableObject {
  @Published var image: UIImage?

  @MainActor func loadThumbnail(assetURL: URL) async throws {
    if assetURL.scheme == "blob" {
      // FIXME: Throw an error? Get favicon as fallback
      return
    }
    if let cachedImage = SDImageCache.shared.imageFromCache(forKey: assetURL.absoluteString) {
      image = cachedImage
      return
    }
    // FIXME: Figure out if we still need to use something else to deal with HLS
    let generator = AVAssetImageGenerator(asset: .init(url: assetURL))
    image = UIImage(
      cgImage: try await generator.image(at: .init(seconds: 3, preferredTimescale: 1)).image
    )
    await SDImageCache.shared.store(image, forKey: assetURL.absoluteString)
  }
}

@available(iOS 16.0, *)
struct MediaThumbnail: View {
  @StateObject private var thumbnailLoader: MediaThumbnailLoader = .init()

  var assetURL: URL

  var body: some View {
    Color.clear
      .overlay {
        if let image = thumbnailLoader.image {
          Image(uiImage: image)
            .resizable()
            .aspectRatio(contentMode: .fill)
        }
      }
      .task(id: assetURL) {
        do {
          try await thumbnailLoader.loadThumbnail(assetURL: assetURL)
        } catch {
          // FIXME: Fetch favicon as fallback
        }
      }
  }
}
