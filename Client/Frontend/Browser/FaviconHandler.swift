/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import Storage
import SDWebImage
import class Data.FaviconMO
import UIKit

class FaviconHandler {
  static let maximumFaviconSize = 1 * 1024 * 1024  // 1 MiB file size limit

  private var tabObservers: TabObservers!
  private let backgroundQueue = OperationQueue()

  init() {
    self.tabObservers = registerFor(.didLoadPageMetadata, queue: backgroundQueue)
  }

  deinit {
    unregister(tabObservers)
  }

  @MainActor func loadFaviconURL(
    _ faviconURL: String,
    type: IconType,
    forTab tab: Tab
  ) async throws -> (Favicon, Data?) {
    guard let iconURL = URL(string: faviconURL), let currentURL = tab.url else {
      throw FaviconError.noImageLoaded
    }
    
    var imageOperation: SDWebImageOperation?

    let webImageCache = WebImageCacheManager.shared
    
    return await withCheckedContinuation { continuation in
      let onProgress: ImageCacheProgress = { receivedSize, expectedSize, _ in
        if receivedSize >= FaviconHandler.maximumFaviconSize || expectedSize > FaviconHandler.maximumFaviconSize {
          imageOperation?.cancel()
        }
      }
      
      let onSuccess: (Favicon, Data?) -> Void = { [weak tab] (favicon, data) -> Void in
        defer { continuation.resume(returning: (favicon, data)) }
        
        guard let tab = tab else { return }
        
        tab.favicons.append(favicon)
        FaviconMO.add(favicon, forSiteUrl: currentURL, persistent: !tab.isPrivate)
      }
      
      let onCompletedSiteFavicon: ImageCacheCompletion = { [weak tab] image, data, _, _, url in
        let favicon = Favicon(url: url.absoluteString, date: Date(), type: type)
        
        guard let image = image else {
          favicon.width = 0
          favicon.height = 0
          onSuccess(favicon, data)
          return
        }
        
        if let tab = tab, !tab.isPrivate {
          webImageCache.cacheImage(image: image, data: data ?? image.pngData(), url: url)
        }
        
        if let header = "%PDF".data(using: .utf8),
           let imageData = data,
           imageData.count >= header.count,
           let range = imageData.range(of: header),
           range.lowerBound.distance(to: imageData.startIndex) < 8 {
          // strict PDF parsing. Otherwise index <= (1024 - header.count)
          // ^8 is the best range because some PDF's can contain a UTF-8 BOM (Byte-Order Mark)
          
          favicon.width = 0
          favicon.height = 0
          onSuccess(favicon, data)
          return
        }
        
        favicon.width = Int(image.size.width)
        favicon.height = Int(image.size.height)
        onSuccess(favicon, data)
      }
      
      let onCompletedPageFavicon: ImageCacheCompletion = { [weak tab] image, data, _, _, url in
        guard let image = image else {
          // If we failed to download a page-level icon, try getting the domain-level icon
          // instead before ultimately failing.
          let siteIconURL = currentURL.domainURL.appendingPathComponent("favicon.ico")
          imageOperation = webImageCache.load(from: siteIconURL, options: [.lowPriority], progress: onProgress, completion: onCompletedSiteFavicon)
          return
        }
        
        let favicon = Favicon(url: url.absoluteString, date: Date(), type: type)
        favicon.width = Int(image.size.width)
        favicon.height = Int(image.size.height)
        
        if let tab = tab, !tab.isPrivate {
          // Somehow `image` is valid and never null, but `data` can be! (On websites that return `.ico` for example.
          webImageCache.cacheImage(image: image, data: data ?? image.pngData(), url: url)
        }
        
        onSuccess(favicon, data)
      }
      
      imageOperation = webImageCache.load(from: iconURL, options: [.lowPriority], progress: onProgress, completion: onCompletedPageFavicon)
    }
  }
}

extension FaviconHandler: TabEventHandler {
  func tab(_ tab: Tab, didLoadPageMetadata metadata: PageMetadata) {
    tab.favicons.removeAll(keepingCapacity: false)
    Task { @MainActor in
      if let iconURL = metadata.largeIconURL {
        let (favicon, data) = try await loadFaviconURL(iconURL, type: .appleIcon, forTab: tab)
        TabEvent.post(.didLoadFavicon(favicon, with: data), for: tab)
      } else if let iconURL = metadata.faviconURL {
        let (favicon, data) = try await loadFaviconURL(iconURL, type: .appleIcon, forTab: tab)
        TabEvent.post(.didLoadFavicon(favicon, with: data), for: tab)
      }
      // No favicon fetched from metadata, trying base domain's standard favicon location.
      else if let baseURL = tab.url?.domainURL {
        let (favicon, data) = try await loadFaviconURL(
          baseURL.appendingPathComponent("favicon.ico").absoluteString,
          type: .icon, forTab: tab)
        TabEvent.post(.didLoadFavicon(favicon, with: data), for: tab)
      }
    }
  }
}

enum FaviconError: Error {
  case noImageLoaded
}
