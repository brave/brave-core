// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveCore
import SDWebImage
import Shared
import os.log

/// Favicon Errors
public enum FaviconError: Error {
  case noImagesFound
  case noBundledImages
}

/// Handles obtaining favicons for URLs from local files, database or internet
public class FaviconFetcher {

  /// The size requirement for the favicon
  public enum Kind {
    /// Load favicons marked as `apple-touch-icon`.
    ///
    /// Usage: NTP, Favorites
    case largeIcon
    /// Load smaller favicons
    ///
    /// Usage: History, Search, Tab Tray
    case smallIcon
  }

  public static func clearCache() {
    SDImageCache.shared.memoryCache.removeAllObjects()
    SDImageCache.shared.diskCache.removeAllData()
  }

  @MainActor
  public static func loadIcon(url: URL, kind: FaviconFetcher.Kind = .smallIcon, persistent: Bool) async throws -> Favicon {
    if let favicon = getFromCache(for: url) {
      return favicon
    }

    try Task.checkCancellation()

    // Fetch the Brave-Core icons
    if let favicon = try? await FaviconRenderer.loadIcon(for: url, persistent: persistent), !favicon.isMonogramImage {
      storeInCache(favicon, for: url, persistent: persistent)
      try Task.checkCancellation()
      return favicon
    }

    // Fetch Bundled or Custom icons
    // If there is an error, we'll try to fetch the cached icons
    if let favicon = try? await BundledFaviconRenderer.loadIcon(url: url) {
      storeInCache(favicon, for: url, persistent: persistent)
      try Task.checkCancellation()
      return favicon
    }

    throw FaviconError.noImagesFound
  }

  private static func cacheURL(for url: URL) -> URL {
    // Some websites still only have a favicon for the FULL url including the fragmented parts
    // But they won't have a favicon for their domain
    // In this case, we want to store the favicon for the entire domain regardless of query parameters or fragmented parts
    // Example: `https://app.uniswap.org/` has no favicon, but `https://app.uniswap.org/#/swap?chain=mainnet` does.
    return URLOrigin(url: url).url ?? url
  }

  private static func storeInCache(_ favicon: Favicon, for url: URL, persistent: Bool) {
    // Do not cache non-persistent icons
    // Do not cache monogram icons
    if persistent && !favicon.isMonogramImage {
      do {
        let data = try JSONEncoder().encode(favicon)
        let cachedURL = cacheURL(for: url)
        SDImageCache.shared.memoryCache.setObject(data, forKey: cachedURL.absoluteString, cost: UInt(data.count))
        SDImageCache.shared.diskCache.setData(data, forKey: cachedURL.absoluteString)
      } catch {
        Logger.module.error("Error Caching Favicon: \(error)")
      }
    }
  }

  private static func getFromCache(for url: URL) -> Favicon? {
    let cachedURL = cacheURL(for: url)
    guard let data = SDImageCache.shared.memoryCache.object(forKey: cachedURL.absoluteString) as? Data ??
            SDImageCache.shared.diskCache.data(forKey: cachedURL.absoluteString) else {
      return nil
    }

    do {
      return try JSONDecoder().decode(Favicon.self, from: data)
    } catch {
      Logger.module.error("Error Decoding Favicon: \(error)")
    }
    return nil
  }
}
