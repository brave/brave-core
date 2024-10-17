// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import FaviconModels
import Foundation
import SDWebImage
import Shared
import UIKit
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

  public static func clearCache() async {
    SDImageCache.shared.memoryCache.removeAllObjects()
    SDImageCache.shared.diskCache.removeAllData()
  }

  /// Fetch a favicon in the following order:
  /// 1. Fetch from Cache
  /// 2. Fetch from Brave-Core
  /// 3. Fetch from Bundled Icons
  /// 4. Fetch Monogram Icons
  /// Notes: Does NOT make a request to fetch icons from the page.
  ///      Requests are only made in FaviconScriptHandler, when the user visits the page.
  public static func loadIcon(
    url: URL,
    kind: FaviconFetcher.Kind = .smallIcon,
    persistent: Bool
  ) async throws -> Favicon {
    try Task.checkCancellation()

    if let favicon = await getFromCache(for: url) {
      return favicon
    }

    // Fetch the Brave-Core icons
    let favicon = try? await FaviconRenderer.loadIcon(for: url, persistent: persistent)
    if let favicon = favicon, !favicon.isMonogramImage {
      await storeInCache(favicon, for: url, persistent: persistent)
      try Task.checkCancellation()
      return favicon
    }

    // Fetch Bundled or Custom icons
    // If there is an error, we'll try to fetch the cached icons
    if let favicon = try? await BundledFaviconRenderer.loadIcon(url: url) {
      await storeInCache(favicon, for: url, persistent: true)
      try Task.checkCancellation()
      return favicon
    }

    // Cache and return Monogram icons
    if let favicon = favicon {
      await storeInCache(favicon, for: url, persistent: persistent)
      return favicon
    }

    // No icons were found
    throw FaviconError.noImagesFound
  }

  /// Creates a monogram Favicon with the following conditions
  /// 1. If `monogramString` is not null, it is used to render the Favicon image.
  /// 2. If `monogramString` is null, the first character of the URL's domain is used to render the Favicon image.
  public static func monogramIcon(
    url: URL,
    monogramString: Character? = nil,
    persistent: Bool
  ) async throws -> Favicon {
    try Task.checkCancellation()

    if let favicon = await getFromCache(for: url) {
      return favicon
    }

    // Render the Monogram on a UIImage
    guard let attributes = BraveCore.FaviconAttributes.withDefaultImage() else {
      throw FaviconError.noImagesFound
    }

    let textColor = !attributes.isDefaultBackgroundColor ? attributes.textColor : nil
    let backColor = !attributes.isDefaultBackgroundColor ? attributes.backgroundColor : nil
    var monogramText = attributes.monogramString
    if let monogramString = monogramString ?? url.baseDomain?.first {
      monogramText = String(monogramString)
    }

    let favicon = await UIImage.renderMonogram(
      url,
      textColor: textColor,
      backgroundColor: backColor,
      monogramString: monogramText
    )
    await storeInCache(favicon, for: url, persistent: persistent)
    try Task.checkCancellation()
    return favicon
  }

  /// Retrieves a Favicon from the cache
  public static func getIconFromCache(for url: URL) async -> Favicon? {
    // Handle internal URLs
    var url = url
    if let internalURL = InternalURL(url),
      let realUrl = internalURL.extractedUrlParam
    {
      url = realUrl
    }

    // Fetch from cache
    if let favicon = await getFromCache(for: url) {
      return favicon
    }

    // When we search for a domain in the URL bar,
    // it automatically makes the scheme `http`
    // Even if the website loads/redirects as `https`
    // Attempt to use `https` favicons if they exist
    if url.scheme == "http", var components = URLComponents(string: url.absoluteString) {
      components.scheme = "https"

      // Fetch from cache
      if let url = components.url, let favicon = await FaviconFetcher.getIconFromCache(for: url) {
        return favicon
      }
    }

    return nil
  }

  /// Updates the Favicon in the cache with the specified icon if any, otherwise removes the favicon from the cache.
  public static func updateCache(_ favicon: Favicon?, for url: URL, persistent: Bool) async {
    guard let favicon, !favicon.isMonogramImage else {
      let cachedURL = cacheURL(for: url)
      SDImageCache.shared.memoryCache.removeObject(forKey: cachedURL.absoluteString)
      SDImageCache.shared.diskCache.removeData(forKey: cachedURL.absoluteString)
      return
    }

    await storeInCache(favicon, for: url, persistent: persistent)
  }

  /// Delete the favicon from disk and memory cache for the given URL
  public static func deleteCache(for url: URL) async {
    let cachedURL = cacheURL(for: url)
    SDImageCache.shared.memoryCache.removeObject(forKey: cachedURL.absoluteString)
    SDImageCache.shared.diskCache.removeData(forKey: cachedURL.absoluteString)
  }

  private static func cacheURL(for url: URL) -> URL {
    // Some websites still only have a favicon for the FULL url including the fragmented parts
    // But they won't have a favicon for their domain
    // In this case, we want to store the favicon for the entire domain regardless of query parameters or fragmented parts
    // Example: `https://app.uniswap.org/` has no favicon, but `https://app.uniswap.org/#/swap?chain=mainnet` does.
    return url.domainURL
  }

  private static func storeInCache(_ favicon: Favicon, for url: URL, persistent: Bool) async {
    // Do not cache non-persistent icons to disk
    if persistent {
      do {
        let cachedURL = cacheURL(for: url)
        SDImageCache.shared.memoryCache.setObject(favicon, forKey: cachedURL.absoluteString)
        SDImageCache.shared.diskCache.setData(
          try JSONEncoder().encode(favicon),
          forKey: cachedURL.absoluteString
        )
      } catch {
        Logger.module.error("Error Caching Favicon: \(error)")
      }
    } else {
      // Cache non-persistent icons to memory only
      SDImageCache.shared.memoryCache.setObject(favicon, forKey: cacheURL(for: url).absoluteString)
    }
  }

  private static func getFromCache(for url: URL) async -> Favicon? {
    let cachedURL = cacheURL(for: url)
    if let favicon = SDImageCache.shared.memoryCache.object(forKey: cachedURL.absoluteString)
      as? Favicon
    {
      return favicon
    }

    guard let data = SDImageCache.shared.diskCache.data(forKey: cachedURL.absoluteString) else {
      return nil
    }

    do {
      let favicon = try JSONDecoder().decode(Favicon.self, from: data)
      SDImageCache.shared.memoryCache.setObject(favicon, forKey: cachedURL.absoluteString)
      return favicon
    } catch {
      Logger.module.error("Error Decoding Favicon: \(error)")
    }
    return nil
  }
}
